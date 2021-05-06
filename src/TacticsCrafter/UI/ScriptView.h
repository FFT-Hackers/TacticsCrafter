#ifndef TC_UI_SCRIPT_VIEW_H
#define TC_UI_SCRIPT_VIEW_H

#include <QWidget>
#include <QLabel>

class Script;
class ScriptView : public QWidget
{
    Q_OBJECT

public:
    ScriptView(QWidget* parent = nullptr);

    void setScript(Script* script);
    void refresh();

private:
    Script* _script;
    QLabel* _labelName;
    QLabel* _labelVersion;
    QLabel* _labelAuthor;
    QLabel* _labelDescription;
};

#endif /* TC_UI_SCRIPT_VIEW_H */