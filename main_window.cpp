#include "main_window.h"
#include "spell_checker.h"
#include "ui_find_replace_dialog.h"
#include "ui_word_frequency_dialog.h"

#include <QAction>
#include <QApplication>
#include <QBrush>
#include <QContextMenuEvent>
#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QFont>
#include <QHeaderView>
#include <QIcon>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPixmap>
#include <QRegularExpression>
#include <QStatusBar>
#include <QTableWidgetItem>
#include <QTextCharFormat>
#include <QTextDocument>
#include <QTextStream>
#include <QToolBar>
#include <QToolButton>
#include <QPrinter>
#include <QPrintDialog>
#include <QPrintPreviewDialog>

#include <algorithm>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

main_window::main_window() {
    setWindowTitle("Notepad");
    resize(800, 600);

    editor = new QTextEdit(this);
    setCentralWidget(editor);

    transforms.push_back(std::make_unique<uppercase_transform>());
    transforms.push_back(std::make_unique<lowercase_transform>());
    transforms.push_back(std::make_unique<capitalize_transform>());
    transforms.push_back(std::make_unique<sentence_case_transform>());
    transforms.push_back(std::make_unique<swap_case_transform>());

    checker = new spell_checker(editor->document());

    std::vector<std::string> paths = {
        "data/words.txt",
        "../data/words.txt",
        "../../data/words.txt",
        "C:/Users/samir/CLionProjects/simple-notepad-project/data/words.txt"
    };

    bool loaded = false;
    for (const auto &path: paths) {
        checker->load_dictionary(path);
        if (checker->is_correct("hello")) {
            loaded = true;
            qDebug() << "Dictionary loaded from:" << path.c_str();
            break;
        }
    }

    if (!loaded) {
        qDebug() << "Failed to load dictionary from any path";
    }

    editor->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(editor, &QTextEdit::customContextMenuRequested, this, &main_window::show_context_menu);

    setup_file_menu();
    setup_edit_menu();
    setup_format_menu();
    setup_format_toolbar();
    setup_search_menu();
    setup_status_bar();
    setup_tools_menu();

    connect(editor, &QTextEdit::textChanged, this, &main_window::update_word_and_line_count);
    update_word_and_line_count();
    connect(editor, &QTextEdit::cursorPositionChanged, this, &main_window::update_cursor_position);
    connect(editor, &QTextEdit::textChanged, this, &main_window::update_cursor_position);
    update_cursor_position();
}

main_window::~main_window() = default;

void main_window::setup_file_menu() {
    auto *file_menu = menuBar()->addMenu("File");

    QAction *action_new = file_menu->addAction("New");
    connect(action_new, &QAction::triggered, this, [this] {
        editor->clear();
        current_file.clear();
        update_title();
    });

    file_menu->addSeparator();

    QAction *action_open = file_menu->addAction("Open...");
    connect(action_open, &QAction::triggered, this, [this] { open_file(); });

    QAction *action_save = file_menu->addAction("Save");
    connect(action_save, &QAction::triggered, this, [this] { save_file(); });

    QAction *action_save_as = file_menu->addAction("Save As...");
    connect(action_save_as, &QAction::triggered, this, [this] { save_file_as(); });

    file_menu->addSeparator();

    QAction *action_print = file_menu->addAction("Print...");
    action_print->setShortcut(QKeySequence::Print);
    connect(action_print, &QAction::triggered, this, &main_window::print_document);

    file_menu->addSeparator();

    QAction *action_exit = file_menu->addAction("Exit");
    connect(action_exit, &QAction::triggered, this, [] { QApplication::quit(); });
}

void main_window::setup_edit_menu() {
    auto *edit_menu = menuBar()->addMenu("Edit");

    QAction *action_undo = edit_menu->addAction("Undo");
    action_undo->setShortcut(QKeySequence::Undo);
    connect(action_undo, &QAction::triggered, editor, &QTextEdit::undo);

    QAction *action_redo = edit_menu->addAction("Redo");
    action_redo->setShortcut(QKeySequence::Redo);
    connect(action_redo, &QAction::triggered, editor, &QTextEdit::redo);

    edit_menu->addSeparator();

    QAction *action_cut = edit_menu->addAction("Cut");
    action_cut->setShortcut(QKeySequence::Cut);
    connect(action_cut, &QAction::triggered, editor, &QTextEdit::cut);

    QAction *action_copy = edit_menu->addAction("Copy");
    action_copy->setShortcut(QKeySequence::Copy);
    connect(action_copy, &QAction::triggered, editor, &QTextEdit::copy);

    QAction *action_paste = edit_menu->addAction("Paste");
    action_paste->setShortcut(QKeySequence::Paste);
    connect(action_paste, &QAction::triggered, editor, &QTextEdit::paste);

    edit_menu->addSeparator();

    QAction *action_select_all = edit_menu->addAction("Select All");
    action_select_all->setShortcut(QKeySequence::SelectAll);
    connect(action_select_all, &QAction::triggered, editor, &QTextEdit::selectAll);
}

void main_window::setup_format_menu() {
    auto *format_menu = menuBar()->addMenu("Format");
    auto *text_case_menu = format_menu->addMenu("Text Case");

    for (const auto &transform: transforms) {
        QAction *action = text_case_menu->addAction(QString::fromStdString(transform->name()));
        connect(action, &QAction::triggered, this, [this, &transform] {
            apply_transform(*transform);
        });
    }
}

void main_window::set_alignment_left() {
    editor->setAlignment(Qt::AlignLeft);
}

void main_window::set_alignment_center() {
    editor->setAlignment(Qt::AlignCenter);
}

void main_window::set_alignment_right() {
    editor->setAlignment(Qt::AlignRight);
}

void main_window::set_text_size(int size) {
    QTextCharFormat format;
    format.setFontPointSize(size);

    if (editor->textCursor().hasSelection()) {
        editor->textCursor().mergeCharFormat(format);
    } else {
        editor->setCurrentCharFormat(format);
    }
}

void main_window::setup_format_toolbar() {
    auto *toolbar = addToolBar("Format");
    toolbar->setIconSize(QSize(16, 16));

    QAction *action_bold = toolbar->addAction(QIcon("data/images/bold.svg"), "Bold");
    action_bold->setToolTip("Bold (Ctrl+B)");
    action_bold->setCheckable(true);
    action_bold->setShortcut(QKeySequence("Ctrl+B"));
    connect(action_bold, &QAction::triggered, this, [this](bool checked) {
        QTextCharFormat fmt;
        fmt.setFontWeight(checked ? QFont::Bold : QFont::Normal);
        editor->mergeCurrentCharFormat(fmt);
    });

    QAction *action_italic = toolbar->addAction(QIcon("data/images/italic.svg"), "Italic");
    action_italic->setToolTip("Italic (Ctrl+I)");
    action_italic->setCheckable(true);
    action_italic->setShortcut(QKeySequence("Ctrl+I"));
    connect(action_italic, &QAction::triggered, this, [this](bool checked) {
        QTextCharFormat fmt;
        fmt.setFontItalic(checked);
        editor->mergeCurrentCharFormat(fmt);
    });

    QAction *action_underline = toolbar->addAction(QIcon("data/images/underline.svg"), "Underline");
    action_underline->setToolTip("Underline (Ctrl+U)");
    action_underline->setCheckable(true);
    action_underline->setShortcut(QKeySequence("Ctrl+U"));
    connect(action_underline, &QAction::triggered, this, [this](bool checked) {
        QTextCharFormat fmt;
        fmt.setFontUnderline(checked);
        editor->mergeCurrentCharFormat(fmt);
    });

    toolbar->addSeparator();

    QMenu *color_menu = new QMenu(this);

    struct ColorItem {
        QString name;
        QColor color;
    };

    QVector<ColorItem> colors = {
        {"Black", Qt::black},
        {"Red", Qt::red},
        {"Blue", Qt::blue},
        {"Green", Qt::green},
        {"Orange", QColor(255, 165, 0)},
        {"Purple", QColor(128, 0, 128)},
        {"Brown", QColor(139, 69, 19)},
        {"Cyan", Qt::cyan}
    };

    for (const auto &item: colors) {
        QAction *color_action_item = color_menu->addAction(item.name);

        QPixmap pixmap(16, 16);
        pixmap.fill(item.color);
        color_action_item->setIcon(QIcon(pixmap));

        connect(color_action_item, &QAction::triggered, this, [this, item]() {
            QTextCursor cursor = editor->textCursor();

            QTextCharFormat currentFormat = cursor.hasSelection() ? cursor.charFormat() : editor->currentCharFormat();

            QTextCharFormat newFormat = currentFormat;
            newFormat.setForeground(QBrush(item.color));

            if (cursor.hasSelection()) {
                cursor.mergeCharFormat(newFormat);
            } else {
                editor->setCurrentCharFormat(newFormat);
            }

            if (color_action) {
                QPixmap color_pixmap(16, 16);
                color_pixmap.fill(item.color);
                color_action->setIcon(QIcon(color_pixmap));
            }
        });
    }

    color_action = new QAction(this);

    QPixmap default_pixmap(16, 16);
    default_pixmap.fill(Qt::black);
    color_action->setIcon(QIcon(default_pixmap));
    color_action->setToolTip("Text Color");

    QToolButton *color_button = new QToolButton(this);
    color_button->setDefaultAction(color_action);
    color_button->setPopupMode(QToolButton::InstantPopup);
    color_button->setMenu(color_menu);
    color_button->setIconSize(QSize(16, 16));

    toolbar->addWidget(color_button);

    toolbar->addSeparator();

    QAction *action_align_left = toolbar->addAction("⬅️");
    action_align_left->setToolTip("Align Left");
    action_align_left->setCheckable(true);

    QAction *action_align_center = toolbar->addAction("⬌");
    action_align_center->setToolTip("Center");
    action_align_center->setCheckable(true);

    QAction *action_align_right = toolbar->addAction("➡️");
    action_align_right->setToolTip("Align Right");
    action_align_right->setCheckable(true);

    connect(action_align_left, &QAction::triggered, this,
            [this, action_align_left, action_align_center, action_align_right]() {
                set_alignment_left();
                action_align_left->setChecked(true);
                action_align_center->setChecked(false);
                action_align_right->setChecked(false);
            });

    connect(action_align_center, &QAction::triggered, this,
            [this, action_align_left, action_align_center, action_align_right]() {
                set_alignment_center();
                action_align_left->setChecked(false);
                action_align_center->setChecked(true);
                action_align_right->setChecked(false);
            });

    connect(action_align_right, &QAction::triggered, this,
            [this, action_align_left, action_align_center, action_align_right]() {
                set_alignment_right();
                action_align_left->setChecked(false);
                action_align_center->setChecked(false);
                action_align_right->setChecked(true);
            });
    toolbar->addSeparator();

    QAction *action_text_size = toolbar->addAction("12");
    action_text_size->setToolTip("Text Size");

    QToolButton *size_button = new QToolButton(this);
    size_button->setDefaultAction(action_text_size);
    size_button->setPopupMode(QToolButton::InstantPopup);

    QMenu *size_menu = new QMenu(this);
    int sizes[] = {6, 8, 10, 12, 14, 16, 18, 20};

    for (int size: sizes) {
        QAction *size_action = size_menu->addAction(QString::number(size));
        connect(size_action, &QAction::triggered, this, [this, size, action_text_size]() {
            QTextCursor cursor = editor->textCursor();

            QTextCharFormat currentFormat;
            if (cursor.hasSelection()) {
                currentFormat = cursor.charFormat();
            } else {
                currentFormat = editor->currentCharFormat();
            }

            QTextCharFormat newFormat = currentFormat;
            newFormat.setFontPointSize(size);

            if (cursor.hasSelection()) {
                cursor.mergeCharFormat(newFormat);
            } else {
                editor->setCurrentCharFormat(newFormat);
            }

            action_text_size->setText(QString::number(size));
        });
    }
    size_button->setMenu(size_menu);
    toolbar->addWidget(size_button);
}

void main_window::setup_search_menu() {
    auto *search_menu = menuBar()->addMenu("Search");

    QAction *action_find_replace = search_menu->addAction("Find / Replace...");
    action_find_replace->setShortcut(QKeySequence::Find);
    connect(action_find_replace, &QAction::triggered, this, [this] {
        show_find_replace_dialog();
    });
}

void main_window::setup_tools_menu() {
    auto *tools_menu = menuBar()->addMenu("Tools");

    QAction *action_word_freq = tools_menu->addAction("Word Frequency...");
    connect(action_word_freq, &QAction::triggered, this, [this] {
        show_word_frequency();
    });

    tools_menu->addSeparator();

    QAction *action_check_spelling = tools_menu->addAction("Check Spelling...");
    connect(action_check_spelling, &QAction::triggered, this, [this] {
        if (checker) {
            checker->rehighlight_all();
            QMessageBox::information(this, "Spell Check",
                                     "Spell check completed. Misspelled words are underlined in red.");
        }
    });
}

void main_window::setup_status_bar() {
    statusBar()->showMessage("Ready");
}

void main_window::open_file() {
    QString path = QFileDialog::getOpenFileName(this, "Open File");
    if (path.isEmpty()) {
        return;
    }

    try {
        QFile file(path);
        if (!file.exists()) {
            throw file_not_found_exception(path.toStdString());
        }
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            throw file_read_exception(path.toStdString());
        }
        QTextStream in(&file);
        QString contents = in.readAll();
        if (in.status() != QTextStream::Ok) {
            throw file_read_exception(path.toStdString());
        }
        editor->setPlainText(contents);
        current_file = path;
        update_title();
    } catch (const notepad_exception &ex) {
        QMessageBox::critical(this, "Error", ex.what());
    }
}

void main_window::save_file() {
    if (current_file.isEmpty()) {
        save_file_as();
        return;
    }

    try {
        QFile file(current_file);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            throw file_write_exception(current_file.toStdString());
        }
        QTextStream out(&file);
        out << editor->toPlainText();
        if (out.status() != QTextStream::Ok) {
            throw file_write_exception(current_file.toStdString());
        }
    } catch (const notepad_exception &ex) {
        QMessageBox::critical(this, "Error", ex.what());
    }
}

void main_window::save_file_as() {
    QString path = QFileDialog::getSaveFileName(this, "Save File As");
    if (path.isEmpty()) {
        return;
    }
    current_file = path;
    save_file();
    update_title();
}

void main_window::print_document() {
    QPrinter printer(QPrinter::HighResolution);

    QPrintPreviewDialog preview(&printer, this);
    preview.setWindowTitle("Print Preview");

    connect(&preview, &QPrintPreviewDialog::paintRequested,
            this, [this](QPrinter *p) {
                editor->print(p);
            });

    if (preview.exec() == QDialog::Accepted) {
        QPrintDialog dialog(&printer, this);
        if (dialog.exec() == QPrintDialog::Accepted) {
            editor->print(&printer);
            QMessageBox::information(this, "Print", "Document sent to printer.");
        }
    }
}

void main_window::update_title() {
    if (current_file.isEmpty()) {
        setWindowTitle("Notepad");
    } else {
        setWindowTitle("Notepad: " + current_file);
    }
}

void main_window::update_cursor_position() {
    QTextCursor cursor = editor->textCursor();

    int line = cursor.blockNumber() + 1;
    int col = cursor.columnNumber() + 1;

    QString position_text = QString("Line: %1, Column: %2").arg(line).arg(col);

    QString text = editor->toPlainText();

    int line_count = 0;
    for (QChar ch: text) {
        if (ch == '\n') line_count++;
    }
    if (!text.isEmpty() && !text.endsWith('\n')) line_count++;

    int word_count = 0;
    bool in_word = false;
    for (QChar ch: text) {
        if (ch.isLetterOrNumber()) {
            if (!in_word) {
                word_count++;
                in_word = true;
            }
        } else {
            in_word = false;
        }
    }

    QString status = QString("%1  |  Words: %2  Lines: %3")
            .arg(position_text)
            .arg(word_count)
            .arg(line_count);
    statusBar()->showMessage(status);
}

void main_window::update_word_and_line_count() {
    QString text = editor->toPlainText();

    int line_count = 0;
    for (QChar ch: text) {
        if (ch == '\n') line_count++;
    }
    if (!text.isEmpty() && !text.endsWith('\n')) line_count++;

    int word_count = 0;
    bool in_word = false;
    for (QChar ch: text) {
        if (ch.isLetterOrNumber()) {
            if (!in_word) {
                word_count++;
                in_word = true;
            }
        } else {
            in_word = false;
        }
    }

    update_cursor_position();

    QString status = QString("Words: %1  Lines: %2").arg(word_count).arg(line_count);
    statusBar()->showMessage(status);
}

void main_window::apply_transform(const text_transform &transform) const {
    QTextCursor cursor = editor->textCursor();
    if (!cursor.hasSelection()) {
        cursor.select(QTextCursor::Document);
    }
    int start = cursor.selectionStart();
    QString selected = cursor.selectedText().replace(QChar::ParagraphSeparator, '\n');
    std::string original = selected.toStdString();
    std::string result = transform.apply(original);

    cursor.beginEditBlock();
    for (size_t i = 0; i < result.size(); ++i) {
        if (original[i] != result[i]) {
            cursor.setPosition(start + static_cast<int>(i));
            cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 1);
            cursor.insertText(QString(QChar(result[i])), cursor.charFormat());
        }
    }
    cursor.endEditBlock();
}

void main_window::show_find_replace_dialog() {
    if (!find_replace_dlg) {
        find_replace_dlg = new QDialog(this);
        find_replace_ui = std::make_unique<Ui::find_replace_dialog>();
        find_replace_ui->setupUi(find_replace_dlg);

        auto current_flags = [this] {
            QTextDocument::FindFlags flags;
            if (find_replace_ui->case_sensitive_check->isChecked()) {
                flags |= QTextDocument::FindCaseSensitively;
            }
            return flags;
        };

        connect(find_replace_ui->find_next_button, &QPushButton::clicked,
                find_replace_dlg, [this, current_flags] {
                    find_next(find_replace_ui->find_input->text(), current_flags());
                });
        connect(find_replace_ui->replace_button, &QPushButton::clicked,
                find_replace_dlg, [this, current_flags] {
                    replace_current(find_replace_ui->find_input->text(),
                                    find_replace_ui->replace_input->text(), current_flags());
                });
        connect(find_replace_ui->replace_all_button, &QPushButton::clicked,
                find_replace_dlg, [this, current_flags] {
                    replace_all(find_replace_ui->find_input->text(),
                                find_replace_ui->replace_input->text(), current_flags());
                });
        connect(find_replace_ui->close_button, &QPushButton::clicked,
                find_replace_dlg, [this] { find_replace_dlg->hide(); });
    }

    find_replace_dlg->show();
    find_replace_dlg->raise();
    find_replace_dlg->activateWindow();
}

void main_window::find_next(const QString &term, QTextDocument::FindFlags flags) const {
    if (term.isEmpty()) return;
    QTextCursor found = editor->document()->find(term, editor->textCursor(), flags);
    if (found.isNull()) {
        QTextCursor start_cursor = editor->textCursor();
        start_cursor.movePosition(QTextCursor::Start);
        found = editor->document()->find(term, start_cursor, flags);
    }
    if (!found.isNull()) {
        editor->setTextCursor(found);
    }
}

void main_window::replace_current(const QString &term, const QString &replacement,
                                  QTextDocument::FindFlags flags) const {
    if (editor->textCursor().hasSelection()) {
        editor->textCursor().insertText(replacement);
    }
    find_next(term, flags);
}

void main_window::replace_all(const QString &term, const QString &replacement,
                              QTextDocument::FindFlags flags) const {
    if (term.isEmpty()) return;
    QTextCursor cursor = editor->textCursor();
    cursor.movePosition(QTextCursor::Start);
    editor->setTextCursor(cursor);

    while (true) {
        QTextCursor found = editor->document()->find(term, editor->textCursor(), flags);
        if (found.isNull()) break;
        editor->setTextCursor(found);
        editor->textCursor().insertText(replacement);
    }
}

void main_window::show_word_frequency() {
    QString text = editor->toPlainText().toLower();
    std::map<std::string, int> freq;
    std::istringstream stream(text.toStdString());
    std::string word;

    while (stream >> word) {
        word.erase(std::remove_if(word.begin(), word.end(),
                                  [](unsigned char c) { return !std::isalpha(c); }), word.end());
        if (!word.empty()) {
            freq[word]++;
        }
    }

    std::vector<std::pair<std::string, int> > sorted_freq(freq.begin(), freq.end());
    std::sort(sorted_freq.begin(), sorted_freq.end(),
              [](const auto &a, const auto &b) { return a.second > b.second; });

    QDialog *dialog = new QDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    Ui::word_frequency_dialog ui;
    ui.setupUi(dialog);

    ui.frequency_table->setRowCount(static_cast<int>(sorted_freq.size()));
    for (int i = 0; i < static_cast<int>(sorted_freq.size()); ++i) {
        const auto &[w, count] = sorted_freq[i];
        ui.frequency_table->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(w)));
        QTableWidgetItem *count_item = new QTableWidgetItem(QString::number(count));
        count_item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        ui.frequency_table->setItem(i, 1, count_item);
    }

    dialog->exec();
}

void main_window::show_context_menu(const QPoint &pos) {
    QTextCursor cursor = editor->cursorForPosition(pos);
    cursor.select(QTextCursor::WordUnderCursor);
    QString selected_word = cursor.selectedText();

    if (!selected_word.isEmpty() && checker && !checker->is_correct_qstring(selected_word)) {
        QStringList suggestions = checker->suggestions_qstring(selected_word, 5);

        // Limit to max 5 suggestions
        if (suggestions.size() > 5) {
            suggestions = suggestions.mid(0, 5);
        }

        if (!suggestions.isEmpty()) {
            QMenu menu(this);
            for (const QString &suggestion: suggestions) {
                QAction *action = menu.addAction(suggestion);
                connect(action, &QAction::triggered, [this, suggestion, cursor]() {
                    replace_word(suggestion, cursor);
                });
            }
            menu.exec(editor->viewport()->mapToGlobal(pos));
            return;
        }
    }

    QMenu default_menu(this);

    QAction *undo_action = default_menu.addAction("Undo");
    undo_action->setShortcut(QKeySequence::Undo);
    connect(undo_action, &QAction::triggered, editor, &QTextEdit::undo);

    QAction *redo_action = default_menu.addAction("Redo");
    redo_action->setShortcut(QKeySequence::Redo);
    connect(redo_action, &QAction::triggered, editor, &QTextEdit::redo);

    default_menu.addSeparator();

    QAction *cut_action = default_menu.addAction("Cut");
    cut_action->setShortcut(QKeySequence::Cut);
    connect(cut_action, &QAction::triggered, editor, &QTextEdit::cut);

    QAction *copy_action = default_menu.addAction("Copy");
    copy_action->setShortcut(QKeySequence::Copy);
    connect(copy_action, &QAction::triggered, editor, &QTextEdit::copy);

    QAction *paste_action = default_menu.addAction("Paste");
    paste_action->setShortcut(QKeySequence::Paste);
    connect(paste_action, &QAction::triggered, editor, &QTextEdit::paste);

    default_menu.addSeparator();

    QAction *select_all_action = default_menu.addAction("Select All");
    select_all_action->setShortcut(QKeySequence::SelectAll);
    connect(select_all_action, &QAction::triggered, editor, &QTextEdit::selectAll);

    default_menu.exec(editor->viewport()->mapToGlobal(pos));
}

void main_window::replace_word(const QString &new_word, const QTextCursor &cursor) {
    QTextCursor edit_cursor = cursor;
    edit_cursor.select(QTextCursor::WordUnderCursor);
    edit_cursor.insertText(new_word);
}
