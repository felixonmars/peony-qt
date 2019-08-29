#include "peony-search-vfs-file-enumerator.h"
#include "peony-search-vfs-file.h"
#include "file-enumerator.h"
#include <QDebug>

//G_DEFINE_TYPE(PeonySearchVFSFileEnumerator, peony_search_vfs_file_enumerator, G_TYPE_FILE_ENUMERATOR)

G_DEFINE_TYPE_WITH_PRIVATE(PeonySearchVFSFileEnumerator,
                           peony_search_vfs_file_enumerator,
                           G_TYPE_FILE_ENUMERATOR)

static void peony_search_vfs_file_enumerator_parse_uri(PeonySearchVFSFileEnumerator *enumerator,
                                                       const char *uri);

static gboolean peony_search_vfs_file_enumerator_is_file_match(PeonySearchVFSFileEnumerator *enumerator, std::shared_ptr<Peony::FileInfo> info);

/* -- init -- */

static void peony_search_vfs_file_enumerator_init(PeonySearchVFSFileEnumerator *self)
{
    PeonySearchVFSFileEnumeratorPrivate *priv = (PeonySearchVFSFileEnumeratorPrivate*)peony_search_vfs_file_enumerator_get_instance_private(self);
    self->priv = priv;

    self->priv->search_vfs_directory_uri = new QString;
    self->priv->enumerate_queue = new QQueue<std::shared_ptr<Peony::FileInfo>>;
    self->priv->recursive = false;
    self->priv->save_result = false;
    self->priv->case_sensitive = true;
    self->priv->match_name_or_content = true;
}

static void enumerator_dispose(GObject *object);

static GFileInfo *enumerate_next_file(GFileEnumerator *enumerator,
                                      GCancellable *cancellable,
                                      GError **error);

/// async method is modified from glib source file gfileenumerator.c
static void
enumerate_next_files_async (GFileEnumerator     *enumerator,
                            int                  num_files,
                            int                  io_priority,
                            GCancellable        *cancellable,
                            GAsyncReadyCallback  callback,
                            gpointer             user_data);

static GList* //GFileInfo*
enumerate_next_files_finished(GFileEnumerator                *enumerator,
                              GAsyncResult                   *result,
                              GError                        **error);


static gboolean enumerator_close(GFileEnumerator *enumerator,
                                 GCancellable *cancellable,
                                 GError **error);

static void peony_search_vfs_file_enumerator_class_init (PeonySearchVFSFileEnumeratorClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    GFileEnumeratorClass *enumerator_class = G_FILE_ENUMERATOR_CLASS(klass);

    gobject_class->dispose = enumerator_dispose;

    enumerator_class->next_file = enumerate_next_file;

    //async
    enumerator_class->next_files_async = enumerate_next_files_async;
    enumerator_class->next_files_finish = enumerate_next_files_finished;

    enumerator_class->close_fn = enumerator_close;
}

void enumerator_dispose(GObject *object)
{
    PeonySearchVFSFileEnumerator *self = PEONY_SEARCH_VFS_FILE_ENUMERATOR(object);

    delete self->priv->name_regexp;
    delete self->priv->content_regexp;
    delete self->priv->search_vfs_directory_uri;
    self->priv->enumerate_queue->clear();
    delete self->priv->enumerate_queue;
}

static GFileInfo *enumerate_next_file(GFileEnumerator *enumerator,
                                      GCancellable *cancellable,
                                      GError **error)
{
    qDebug()<<"next file";
    if (cancellable) {
        if (g_cancellable_is_cancelled(cancellable)) {
            //FIXME: how to add translation here? do i have to use gettext?
            *error = g_error_new_literal(G_IO_ERROR, G_IO_ERROR_CANCELLED, "search is cancelled");
            return nullptr;
        }
    }
    auto search_enumerator = PEONY_SEARCH_VFS_FILE_ENUMERATOR(enumerator);
    auto enumerate_queue = search_enumerator->priv->enumerate_queue;
    while (!enumerate_queue->isEmpty()) {
        //BFS enumeration
        auto info = enumerate_queue->dequeue();
        if (info->isDir() && search_enumerator->priv->recursive) {
            Peony::FileEnumerator e;
            e.setEnumerateDirectory(info->uri());
            e.enumerateSync();
            enumerate_queue->append(e.getChildren());
        }
        //match
        if (peony_search_vfs_file_enumerator_is_file_match(search_enumerator, info)) {
            //return this info, and the enumerate get child will return the
            //file crosponding the real uri, due to it would be handled in
            //vfs looking up method callback in registed vfs.
            auto search_vfs_info = g_file_info_new();
            QString realUriSuffix = "real-uri:" + info->uri();
            g_file_info_set_name(search_vfs_info, realUriSuffix.toUtf8().constData());
            return search_vfs_info;
        }
    }
    return nullptr;
}

static void
next_async_op_free (GList *files)
{
    g_list_free_full (files, g_object_unref);
}


static void
next_files_thread (GTask        *task,
                   gpointer      source_object,
                   gpointer      task_data,
                   GCancellable *cancellable)
{
    auto enumerator = G_FILE_ENUMERATOR(source_object);
    int num_files = GPOINTER_TO_INT (task_data);
    GFileEnumeratorClass *c;
    GList *files = NULL;
    GError *error = NULL;
    GFileInfo *info;
    int i;

    c = G_FILE_ENUMERATOR_GET_CLASS (enumerator);
    for (i = 0; i < num_files; i++)
      {
        if (g_cancellable_set_error_if_cancelled (cancellable, &error))
      info = NULL;
        else
      info = c->next_file (enumerator, cancellable, &error);

        if (info == NULL)
      {
        break;
      }
        else
      files = g_list_prepend (files, info);
      }

    if (error)
      g_task_return_error (task, error);
    else
      g_task_return_pointer (task, files, (GDestroyNotify)next_async_op_free);
}

static void
enumerate_next_files_async (GFileEnumerator     *enumerator,
                            int                  num_files,
                            int                  io_priority,
                            GCancellable        *cancellable,
                            GAsyncReadyCallback  callback,
                            gpointer             user_data)
{
    GTask *task;
    task = g_task_new (enumerator, cancellable, callback, user_data);
    g_task_set_source_tag (task, (gpointer)enumerate_next_files_async);
    g_task_set_task_data (task, GINT_TO_POINTER (num_files), NULL);
    g_task_set_priority (task, io_priority);

    g_task_run_in_thread (task, next_files_thread);
    g_object_unref (task);
}

static GList* //GFileInfo*
enumerate_next_files_finished(GFileEnumerator                *enumerator,
                              GAsyncResult                   *result,
                              GError                        **error)
{
    g_return_val_if_fail (g_task_is_valid (result, enumerator), NULL);

    return (GList*)g_task_propagate_pointer (G_TASK (result), error);
}

static gboolean enumerator_close(GFileEnumerator *enumerator,
                                 GCancellable *cancellable,
                                 GError **error)
{
    PeonySearchVFSFileEnumerator *self = PEONY_SEARCH_VFS_FILE_ENUMERATOR(enumerator);

    return true;
}

gboolean peony_search_vfs_file_enumerator_is_file_match(PeonySearchVFSFileEnumerator *enumerator, std::shared_ptr<Peony::FileInfo> file_info)
{
    PeonySearchVFSFileEnumeratorPrivate *details = enumerator->priv;
    if (details->name_regexp->isEmpty() && details->content_regexp->isEmpty())
        return true;

    GFile *file = g_file_new_for_uri(file_info->uri().toUtf8().constData());
    GFileInfo *info = g_file_query_info(file,
                                        G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME,
                                        G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                        nullptr,
                                        nullptr);
    g_object_unref(file);

    if (details->name_regexp->isValid()) {
        char *file_display_name = g_file_info_get_attribute_as_string(info, G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME);
        g_object_unref(info);
        QString displayName = file_display_name;
        g_free(file_display_name);
        if (displayName.contains(*enumerator->priv->name_regexp)) {
            if (details->match_name_or_content) {
                return true;
            }
        }
    }

    if (details->content_regexp->isValid()) {
        //read file stream
        bool content_matched = false;
        while (!content_matched) {
            /// TODO: add content matching function
            content_matched = true;
            //read next line
            //if matched
                //content_matched = true;
        }
        //return content_matched;
    }

    //this may never happend.
    return false;
}
