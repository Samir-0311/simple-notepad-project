#include "spell_checker.h"
#include <QDebug>
#include <QRegularExpression>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

spell_checker::spell_checker(QTextDocument* parent)
    : QSyntaxHighlighter(parent)
{
    misspelled_format.setUnderlineColor(Qt::red);
    misspelled_format.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
}

void spell_checker::load_dictionary(const std::string& path)
{
    std::ifstream file(path);

    if (!file.is_open()) {
        qDebug() << "Failed to open dictionary:" << path.c_str();
        return;
    }

    std::string word;
    int count = 0;

    while (file >> word) {
        std::string normalized = normalize(word);
        if (!normalized.empty()) {
            dictionary.insert(normalized);
            count++;
        }
    }

    file.close();
}

std::string spell_checker::normalize(const std::string& word)
{
    std::string result;
    for (char c : word) {
        unsigned char uc = static_cast<unsigned char>(c);
        if (std::isalpha(uc)) {
            result += static_cast<char>(std::tolower(uc));
        }
    }
    return result;
}

bool spell_checker::is_correct(const std::string& word) const
{
    std::string normalized = normalize(word);
    if (normalized.empty()) {
        return true;
    }
    return dictionary.find(normalized) != dictionary.end();
}

bool spell_checker::is_correct_qstring(const QString& word) const
{
    return is_correct(word.toStdString());
}

void spell_checker::highlightBlock(const QString& text)
{
    QRegularExpression re("\\b[A-Za-z']+\\b");
    QRegularExpressionMatchIterator it = re.globalMatch(text);

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString word = match.captured(0);

        if (!is_correct_qstring(word)) {
            setFormat(match.capturedStart(), match.capturedLength(), misspelled_format);
        }
    }
}

int spell_checker::levenshtein_distance(const std::string& a, const std::string& b)
{
    size_t m = a.size();
    size_t n = b.size();

    std::vector<std::vector<int>> dp(m + 1, std::vector<int>(n + 1));

    for (size_t i = 0; i <= m; ++i) dp[i][0] = static_cast<int>(i);
    for (size_t j = 0; j <= n; ++j) dp[0][j] = static_cast<int>(j);

    for (size_t i = 1; i <= m; ++i) {
        for (size_t j = 1; j <= n; ++j) {
            int cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
            dp[i][j] = std::min({dp[i - 1][j] + 1, dp[i][j - 1] + 1, dp[i - 1][j - 1] + cost});
        }
    }

    return dp[m][n];
}

std::vector<std::string>
spell_checker::suggestions(const std::string& word, std::size_t max_count) const
{
    std::vector<std::pair<int, std::string>> candidates;
    std::string normalized = normalize(word);

    if (normalized.empty()) {
        return {};
    }

    int wordLen = static_cast<int>(normalized.size());

    for (const auto& entry : dictionary) {
        int entryLen = static_cast<int>(entry.size());
        if (std::abs(entryLen - wordLen) > 2) {
            continue;
        }

        int distance = levenshtein_distance(normalized, entry);
        if (distance <= 2) {
            candidates.push_back({distance, entry});
        }
    }

    std::sort(candidates.begin(), candidates.end(),
              [](const auto& a, const auto& b) {
                  if (a.first != b.first) return a.first < b.first;
                  return a.second < b.second;
              });

    std::vector<std::string> result;
    for (size_t i = 0; i < candidates.size() && i < max_count; ++i) {
        result.push_back(candidates[i].second);
    }

    return result;
}

QStringList spell_checker::suggestions_qstring(const QString& word, int max_count) const
{
    std::vector<std::string> stdSuggestions = suggestions(word.toStdString(), max_count);
    QStringList result;
    for (size_t i = 0; i < stdSuggestions.size() && i < static_cast<size_t>(max_count); ++i) {
        result.append(QString::fromStdString(stdSuggestions[i]));
    }
    return result;
}