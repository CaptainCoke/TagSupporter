#include "StringDistance.h"
#include <string>
#include <numeric>

StringDistance::StringDistance(QString strReference, StringDistance::CaseSensitivitiy eCaseSensitivity)
: m_strReference( eCaseSensitivity == CaseInsensitive ? strReference.toUpper() : std::move(strReference) )
, m_eCaseSensitivity(eCaseSensitivity)
{
}

int StringDistance::Levenshtein(const QString &strQuery) const
{
    switch ( m_eCaseSensitivity )
    {
    case CaseInsensitive: return Levenshtein( m_strReference, strQuery.toUpper() );
    case CaseSensitive:   return Levenshtein( m_strReference, strQuery );
    }
}

double StringDistance::NormalizedLevenshtein(const QString &strQuery) const
{
    switch ( m_eCaseSensitivity )
    {
    case CaseInsensitive: return NormalizedLevenshtein( m_strReference, strQuery.toUpper() );
    case CaseSensitive:   return NormalizedLevenshtein( m_strReference, strQuery );
    }
}

int StringDistance::Levenshtein( const std::string &s1, const std::string &s2 )
{
	// To change the type this function manipulates and returns, change
	// the return type and the types of the two variables below.
	int s1len = s1.size();
	int s2len = s2.size();
	
	auto column_start = (decltype(s1len))1;
	
	auto column = new decltype(s1len)[s1len + 1];
	std::iota(column + column_start, column + s1len + 1, column_start);
	
	for (auto x = column_start; x <= s2len; x++) {
		column[0] = x;
		auto last_diagonal = x - column_start;
		for (auto y = column_start; y <= s1len; y++) {
			auto old_diagonal = column[y];
			auto possibilities = {
				column[y] + 1,
				column[y - 1] + 1,
				last_diagonal + (s1[y - 1] == s2[x - 1]? 0 : 1)
			};
			column[y] = std::min(possibilities);
			last_diagonal = old_diagonal;
		}
	}
	auto result = column[s1len];
	delete[] column;
	return result;
}

int StringDistance::Levenshtein( const QString &s1, const QString &s2 )
{
    return Levenshtein( s1.toStdString(), s2.toStdString() );
}

double StringDistance::NormalizedLevenshtein(const std::string &s1, const std::string &s2)
{
    return static_cast<double>(Levenshtein( s1, s2 ))/static_cast<double>(std::max(s1.length(),s2.length()));
}

double StringDistance::NormalizedLevenshtein(const QString &s1, const QString&s2)
{
    return NormalizedLevenshtein( s1.toStdString(), s2.toStdString() );
}
