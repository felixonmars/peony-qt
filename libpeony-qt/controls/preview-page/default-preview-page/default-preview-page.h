#ifndef DEFAULTPREVIEWPAGE_H
#define DEFAULTPREVIEWPAGE_H

#include "peony-core_global.h"
#include "preview-page-plugin-iface.h"
#include <QWidget>

class QGridLayout;

namespace Peony {

/*!
 * \brief The DefaultPreviewPage class
 * \todo
 * Implement a preview page containing file thumbnail and descriptions.
 */
class PEONYCORESHARED_EXPORT DefaultPreviewPage : public QWidget, public PreviewPageIface
{
    Q_OBJECT
public:
    explicit DefaultPreviewPage(QWidget *parent = nullptr);
    ~DefaultPreviewPage() override;

    void prepare(const QString &uri, PreviewType type) override;
    void startPreview() override;
    void cancel() override;
    void closePreviewPage() override;

private:
    QString m_current_uri;
    PreviewType m_current_type;

    QGridLayout *m_layout;
};

}

#endif // DEFAULTPREVIEWPAGE_H
