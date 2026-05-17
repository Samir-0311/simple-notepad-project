#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include "text_transform.h"
#include "notepad_exception.h"

#include <QAction>
#include <QToolButton>
#include <QDialog>
#include <QMainWindow>
#include <QString>
#include <QTextDocument>
#include <QTextEdit>
#include <QColor>
#include <memory>
#include <vector>


class spell_checker;

namespace Ui {
    class find_replace_dialog;
    class word_frequency_dialog;
}

class main_window : public QMainWindow {
    Q_OBJECT

public:
    main_window();
    ~main_window() override;

private slots:
    void show_context_menu(const QPoint& pos);

private:
    void setup_file_menu();
    void setup_edit_menu();
    void setup_format_menu();
    void setup_format_toolbar();
    void setup_search_menu();
    void setup_tools_menu();
    void setup_status_bar();

    void set_alignment_left();
    void set_alignment_center();
    void set_alignment_right();

    void set_text_size(int size);
    void show_text_size_menu();

    void open_file();
    void save_file();
    void save_file_as();
    void update_title();
    void update_word_and_line_count();
    void apply_transform(const text_transform &transform) const;

    void show_find_replace_dialog();
    void find_next(const QString &term, QTextDocument::FindFlags flags = QTextDocument::FindFlags()) const;
    void replace_current(const QString &term, const QString &replacement,
                         QTextDocument::FindFlags flags = QTextDocument::FindFlags()) const;
    void replace_all(const QString &term, const QString &replacement,
                     QTextDocument::FindFlags flags = QTextDocument::FindFlags()) const;
    void show_word_frequency();

    void replace_word(const QString &new_word, const QTextCursor &cursor);

    QTextEdit *editor{nullptr};
    QString current_file;
    std::vector<std::unique_ptr<text_transform>> transforms;

    QDialog *find_replace_dlg{nullptr};
    std::unique_ptr<Ui::find_replace_dialog> find_replace_ui;

    spell_checker *checker{nullptr};

    // Color button
    QAction *color_action{nullptr};
};

#endif // MAIN_WINDOW_H