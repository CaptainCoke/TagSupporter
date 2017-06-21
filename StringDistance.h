#ifndef STRINGDISTANCE_H
#define STRINGDISTANCE_H

#include <QString>

class StringDistance
{
public:
    enum CaseSensitivitiy { CaseSensitive, CaseInsensitive };
    StringDistance( QString strReference, CaseSensitivitiy eCaseSensitivity );
    
    int    Levenshtein( const QString& strQuery ) const;
    double NormalizedLevenshtein( const QString& strQuery ) const;
    
    // returns the number of edit operations to get from s1 to s2
    static int Levenshtein( const std::string& s1, const std::string& s2 );
    static int Levenshtein( const QString& s1, const QString& s2 );
    
    // returns Levenshtein, noramlized by the length of the longer string
    static double NormalizedLevenshtein( const std::string &s1, const std::string &s2 );
    static double NormalizedLevenshtein( const QString &s1, const QString &s2 );
    
protected:
    QString m_strReference;
    bool m_eCaseSensitivity;
};

#endif // STRINGDISTANCE_H
