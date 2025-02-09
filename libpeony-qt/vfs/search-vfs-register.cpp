#include "search-vfs-register.h"
#include "peony-search-vfs-file.h"
#include "peony-search-vfs-file-enumerator.h"
#include "search-vfs-manager.h"

#include <gio/gio.h>
#include <QDebug>

using namespace Peony;

bool is_registed = false;

static GFile *
test_vfs_parse_name (GVfs       *vfs,
                     const char *parse_name,
                     gpointer    user_data)
{
    QString tmp = parse_name;
    if (tmp.contains("real-uri:")) {
        QString realUri = tmp.split("real-uri:").last();
        return g_file_new_for_uri(realUri.toUtf8().constData());
    }
    return peony_search_vfs_file_new_for_uri(parse_name);
}

static GFile *
test_vfs_lookup (GVfs       *vfs,
                 const char *uri,
                 gpointer    user_data)
{
    return test_vfs_parse_name(vfs, uri, user_data);
}

void SearchVFSRegister::registSearchVFS()
{
    if (is_registed)
        return;

    //init manager
    Peony::SearchVFSManager::getInstance();

    GVfs *vfs;
    const gchar * const *schemes;
    gboolean res;

    vfs = g_vfs_get_default ();
    schemes = g_vfs_get_supported_uri_schemes(vfs);
    const gchar * const *p;
    p = schemes;
    while (*p) {
        qDebug()<<*p;
        p++;
    }

    res = g_vfs_register_uri_scheme (vfs, "search",
                                     test_vfs_lookup, NULL, NULL,
                                     test_vfs_parse_name, NULL, NULL);
}

SearchVFSRegister::SearchVFSRegister()
{

}
