#ifndef LCS_H
#define LCS_H

#include <QString>
#include <QList>

namespace Zeal {
namespace Core {

class LCS
{
public:
    LCS();
    LCS(const QString &a, const QString &b);

    QString subsequence() const;
    int length() const;

    QList<int> subsequencePositions(int arg) const;
    double calcDensity(int arg) const;
    double calcSpread(int arg) const;

private:
    QString m_a;
    QString m_b;
    QString m_subsequence;

    int **createLengthMatrix() const;
    void fillLengthMatrix(int **matrix) const;
    void freeLengthMatrix(int **matrix) const;
    QString backtrackLengthMatrix(int **matrix, int i, int j) const;
};

} // namespace Core
} // namespace Zeal

#endif // LCS_H
