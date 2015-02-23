#include "lcs.h"

using namespace Zeal::Core;

LCS::LCS()
{
}

LCS::LCS(const QString &a, const QString &b):
    m_a(a),
    m_b(b)
{
    if (m_a.isEmpty() || m_b.isEmpty())
        return;

    // Uses dynamic programming to calculate and store the size of the longest 
    // common subsequence shared between a & b;
    
    int **lengthMatrix = createLengthMatrix();
    fillLengthMatrix(lengthMatrix);
    m_subsequence = backtrackLengthMatrix(lengthMatrix, m_a.length(), m_b.length());
    freeLengthMatrix(lengthMatrix);
}

QString LCS::subsequence() const
{
    return m_subsequence;
}

int LCS::length() const
{
    return m_subsequence.length();
}

// Metric of how much of the longest common subsequence covers the target 
// string. The more the subsequence and target have in common the higher 
// the desity up until 1 for a perfect match;
double LCS::calcDensity(int arg) const 
{
    if (m_subsequence.length() == 0)
        return 0;
    QString target = arg == 0 ? m_a : m_b;

    return double(m_subsequence.length()) / target.length();
}

// Metric of how chopped the longest common subsequence is against the target.
// The size of the substring composed from the first until the last lcs match
// divided by the size of the LCS. Equals 1 for a subsequence that is actually
// a substring
double LCS::calcSpread(int arg) const
{
    if (m_subsequence.length() == 0)
        return 0;
    QString target = arg == 0 ? m_a : m_b;

    int start = target.indexOf(m_subsequence[0]);
    int end = start;
    for (int i = start, j = 0; j < m_subsequence.length(); i++) {
        if (target[i] == m_subsequence[j]){
            end = i;
            j++;
        }
    }
    end++;

    return double(m_subsequence.length()) / (end - start);
}

// Return lcs positions in target string
QList<int> LCS::subsequencePositions(int arg) const
{
    QString target = arg == 0 ? m_a : m_b;
    QList<int> positions;

    for (int i = 0, j = 0; j < m_subsequence.length(); i++) {
        if (target[i] == m_subsequence[j]){
            positions.append(i);
            j++;
        }
    }

    return positions;
}

int **LCS::createLengthMatrix() const
{
    int rows = m_a.length() + 1;
    int cols = m_b.length() + 1;

    int **matrix = new int*[rows];
    matrix[0] = new int[rows * cols];
    for (int i = 1; i < rows; i++)
        matrix[i] = matrix[0] + i * cols;
    memset(matrix[0], 0, sizeof(matrix[0][0]) * rows * cols);

    return matrix;
}

// Exploits optimal substructure to calculate the size of the longest common 
// subsequnce. Bottom right cell contains the size of the LCS;
void LCS::fillLengthMatrix(int **matrix) const
{
    for (int i = 0; i < m_a.length(); i++) {
        for (int j = 0; j < m_b.length(); j++) {
            if (m_a[i] == m_b[j])
                matrix[i + 1][j + 1] = matrix[i][j] + 1;
            else
                matrix[i + 1][j + 1] = std::max(matrix[i + 1][j], matrix[i][j + 1]);
        }
    }
}

void LCS::freeLengthMatrix(int **matrix) const
{
    delete [] matrix[0];
    delete [] matrix;
}

// Work backwards to actually identify the lcs;
QString LCS::backtrackLengthMatrix(int **matrix, int i, int j) const
{
    if (i == 0 || j == 0)
        return QString();
    else if (m_a[i - 1] == m_b[j - 1]) {
        return backtrackLengthMatrix(matrix, i - 1, j - 1) + m_a[i - 1];
    } else {
        if (matrix[i][j - 1] > matrix[i - 1][j])
            return backtrackLengthMatrix(matrix, i, j - 1);
        else
            return backtrackLengthMatrix(matrix, i - 1, j);
    }
}
