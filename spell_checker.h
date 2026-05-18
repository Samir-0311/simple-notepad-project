#ifndef SPELL_CHECKER_H
#define SPELL_CHECKER_H

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QString>
#include <QStringList>

#include <set>
#include <string>
#include <vector>

class spell_checker : public QSyntaxHighlighter {
    Q_OBJECT

public:
    explicit spell_checker(QTextDocument* parent = nullptr);
    int getDictionarySize() const { return dictionary.size(); }
    void load_dictionary(const std::string& path);

    [[nodiscard]] bool is_correct(const std::string& word) const;
    [[nodiscard]] bool is_correct_qstring(const QString& word) const;

    [[nodiscard]] std::vector<std::string>
    suggestions(const std::string& word, std::size_t max_count = 5) const;

    [[nodiscard]] QStringList suggestions_qstring(const QString& word, int max_count = 5) const;

    void rehighlight_all() { rehighlight(); }

protected:
    void highlightBlock(const QString& text) override;

private:
    static std::string normalize(const std::string& word);
    static int levenshtein_distance(const std::string& a, const std::string& b);

    std::set<std::string> dictionary;
    QTextCharFormat misspelled_format;
};

#endif // SPELL_CHECKER_H