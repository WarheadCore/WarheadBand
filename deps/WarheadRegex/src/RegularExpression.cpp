/*
 * This file is part of the WarheadCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "RegularExpression.h"
#include "RegularExpressionException.h"
#include "pcre2_config.h"
#include "pcre2.h"
#include <sstream>

namespace
{
	class MatchData
	{
	public:
		MatchData(pcre2_code_8* code) :
			_match(pcre2_match_data_create_from_pattern_8(reinterpret_cast<pcre2_code_8*>(code), nullptr))
		{
			if (!_match)
                throw Warhead::RegularExpressionException("cannot create match data");
		}

		~MatchData()
		{
			if (_match)
                pcre2_match_data_free_8(_match);
		}

		std::uint32_t count() const
		{
			return pcre2_get_ovector_count_8(_match);
		}

		const PCRE2_SIZE* data() const
		{
			return pcre2_get_ovector_pointer_8(_match);
		}

		operator pcre2_match_data_8*()
		{
			return _match;
		}

	private:
		pcre2_match_data_8* _match;
	};
}

Warhead::RegularExpression::RegularExpression(std::string_view pattern, int options, bool /*study*/)
{
    if (pattern.empty())
        throw RegularExpressionException("empty pattern");

    pcre2_compile_context_8* context = pcre2_compile_context_create_8(nullptr);
	if (!context)
        throw RegularExpressionException("cannot create compile context");

	if (options & RE_NEWLINE_LF)
		pcre2_set_newline_8(context, PCRE2_NEWLINE_LF);
	else if (options & RE_NEWLINE_CRLF)
		pcre2_set_newline_8(context, PCRE2_NEWLINE_CRLF);
	else if (options & RE_NEWLINE_ANY)
		pcre2_set_newline_8(context, PCRE2_NEWLINE_ANY);
	else if (options & RE_NEWLINE_ANYCRLF)
		pcre2_set_newline_8(context, PCRE2_NEWLINE_ANYCRLF);
	else // default RE_NEWLINE_CR
		pcre2_set_newline_8(context, PCRE2_NEWLINE_CR);

    int errorCode;
    PCRE2_SIZE errorOffset;

    _pcre = pcre2_compile_8(reinterpret_cast<const PCRE2_SPTR>(pattern.data()), pattern.length(), compileOptions(options), &errorCode, &errorOffset, context);
	pcre2_compile_context_free_8(context);

	if (!_pcre)
	{
		PCRE2_UCHAR buffer[256];
		pcre2_get_error_message_8(errorCode, buffer, sizeof(buffer));
		std::ostringstream msg;
		msg << reinterpret_cast<char*>(buffer) << " (at offset " << errorOffset << ")";
		throw RegularExpressionException(msg.str());
	}

    unsigned nameCount;
    unsigned nameEntrySize;
    unsigned char* nameTable;

	pcre2_pattern_info_8(reinterpret_cast<pcre2_code_8*>(_pcre), PCRE2_INFO_NAMECOUNT, &nameCount);
	pcre2_pattern_info_8(reinterpret_cast<pcre2_code_8*>(_pcre), PCRE2_INFO_NAMEENTRYSIZE, &nameEntrySize);
	pcre2_pattern_info_8(reinterpret_cast<pcre2_code_8*>(_pcre), PCRE2_INFO_NAMETABLE, &nameTable);

	for (int i = 0; i < nameCount; i++)
	{
		unsigned char* group = nameTable + 2 + (nameEntrySize * i);
		int n = pcre2_substring_number_from_name_8(reinterpret_cast<pcre2_code_8*>(_pcre), group);
		_groups[n] = std::string(reinterpret_cast<char*>(group));
	}
}

Warhead::RegularExpression::~RegularExpression()
{
	if (_pcre)
        pcre2_code_free_8(reinterpret_cast<pcre2_code_8*>(_pcre));
}

int Warhead::RegularExpression::match(std::string_view subject, std::string::size_type offset, Match& mtch, int options) const
{
    if (offset > subject.length())
        throw RegularExpressionException("offset > subject length");

	MatchData matchData(reinterpret_cast<pcre2_code_8*>(_pcre));
	int rc = pcre2_match_8(reinterpret_cast<pcre2_code_8*>(_pcre), reinterpret_cast<const PCRE2_SPTR>(subject.data()), subject.size(), offset, matchOptions(options), matchData, nullptr);
	if (rc == PCRE2_ERROR_NOMATCH)
	{
		mtch.offset = std::string::npos;
		mtch.length = 0;
		return 0;
	}
	else if (rc == PCRE2_ERROR_BADOPTION)
        throw RegularExpressionException("bad option");
	else if (rc == 0)
        throw RegularExpressionException("too many captured substrings");
	else if (rc < 0)
	{
		PCRE2_UCHAR buffer[256];
		pcre2_get_error_message_8(rc, buffer, sizeof(buffer));
		throw RegularExpressionException(std::string(reinterpret_cast<char*>(buffer)));
	}

    const PCRE2_SIZE* ovec = matchData.data();
	mtch.offset = ovec[0] < 0 ? std::string::npos : ovec[0];
	mtch.length = ovec[1] - mtch.offset;
	return rc;
}

int Warhead::RegularExpression::match(std::string_view subject, std::string::size_type offset, MatchVec& matches, int options) const
{
    if (offset > subject.length())
        throw RegularExpressionException("offset > subject length");

	matches.clear();

	MatchData matchData(reinterpret_cast<pcre2_code_8*>(_pcre));

    int rc = pcre2_match_8(reinterpret_cast<pcre2_code_8*>(_pcre), reinterpret_cast<const PCRE2_SPTR>(subject.data()), subject.size(), offset, options & 0xFFFF, matchData, nullptr);
	if (rc == PCRE2_ERROR_NOMATCH)
        return 0;
	else if (rc == PCRE2_ERROR_BADOPTION)
        throw RegularExpressionException("bad option");
	else if (rc == 0)
        throw RegularExpressionException("too many captured substrings");
	else if (rc < 0)
	{
		PCRE2_UCHAR buffer[256];
		pcre2_get_error_message_8(rc, buffer, sizeof(buffer));
		throw RegularExpressionException(std::string(reinterpret_cast<char*>(buffer)));
	}

    matches.reserve(rc);
	const PCRE2_SIZE* ovec = matchData.data();

    for (int i = 0; i < rc; ++i)
	{
		Match m;
		GroupMap::const_iterator it;

		m.offset = ovec[i * 2] < 0 ? std::string::npos : ovec[i * 2] ;
		m.length = ovec[i * 2 + 1] - m.offset;

		it = _groups.find(i);

        if (it != _groups.end())
            m.name = (*it).second;

		matches.emplace_back(m);
	}

    return rc;
}

bool Warhead::RegularExpression::match(std::string_view subject, std::string::size_type offset) const
{
	Match mtch;
	match(subject, offset, mtch, RE_ANCHORED | RE_NOTEMPTY);
	return mtch.offset == offset && mtch.length == subject.length() - offset;
}

bool Warhead::RegularExpression::match(std::string_view subject, std::string::size_type offset, int options) const
{
	Match mtch;
	match(subject, offset, mtch, options);
	return mtch.offset == offset && mtch.length == subject.length() - offset;
}

int Warhead::RegularExpression::extract(std::string_view subject, std::string& str, int options) const
{
	Match mtch;
	int rc = match(subject, 0, mtch, options);
	if (mtch.offset != std::string::npos)
		str.assign(subject, mtch.offset, mtch.length);
	else
		str.clear();

    return rc;
}

int Warhead::RegularExpression::extract(std::string_view subject, std::string::size_type offset, std::string& str, int options) const
{
	Match mtch;
	int rc = match(subject, offset, mtch, options);
	if (mtch.offset != std::string::npos)
		str.assign(subject, mtch.offset, mtch.length);
	else
		str.clear();

    return rc;
}

int Warhead::RegularExpression::split(std::string_view subject, std::string::size_type offset, std::vector<std::string>& strings, int options) const
{
	MatchVec matches;
	strings.clear();
	int rc = match(subject, offset, matches, options);
	strings.reserve(matches.size());

    for (auto const& m : matches)
	{
        if (m.offset == std::string::npos)
        {
            strings.emplace_back(std::string{});
            continue;
        }

        strings.emplace_back(subject.substr(m.offset, m.length));
	}

    return rc;
}

int Warhead::RegularExpression::subst(std::string& subject, std::string::size_type offset, std::string_view replacement, int options) const
{
	if (options & RE_GLOBAL)
	{
		int rc = 0;
		std::string::size_type pos = substOne(subject, offset, replacement, options);

        while (pos != std::string::npos)
		{
			++rc;
			pos = substOne(subject, pos, replacement, options);
		}

        return rc;
	}
	else
	{
		return substOne(subject, offset, replacement, options) != std::string::npos ? 1 : 0;
	}
}

std::string::size_type Warhead::RegularExpression::substOne(std::string& subject, std::string::size_type offset, std::string_view replacement, int options) const
{
	if (offset >= subject.length())
        return std::string::npos;

	MatchData matchData(reinterpret_cast<pcre2_code_8*>(_pcre));

    int rc = pcre2_match_8(reinterpret_cast<pcre2_code_8*>(_pcre), reinterpret_cast<const PCRE2_SPTR>(subject.c_str()), subject.size(), offset, matchOptions(options), matchData, nullptr);
	if (rc == PCRE2_ERROR_NOMATCH)
        return std::string::npos;
	else if (rc == PCRE2_ERROR_BADOPTION)
        throw RegularExpressionException("bad option");
	else if (rc == 0)
        throw RegularExpressionException("too many captured substrings");
	else if (rc < 0)
	{
		PCRE2_UCHAR buffer[256];
		pcre2_get_error_message_8(rc, buffer, sizeof(buffer));
		throw RegularExpressionException(std::string(reinterpret_cast<char*>(buffer)));
	}

    const PCRE2_SIZE* ovec = matchData.data();
	std::string result;
	std::string::size_type len = subject.length();
	std::string::size_type pos = 0;
	std::string::size_type rp = std::string::npos;

    while (pos < len)
	{
		if (ovec[0] == pos)
		{
			auto it  = replacement.begin();
			auto end = replacement.end();

            while (it != end)
			{
				if (*it == '$' && !(options & RE_NO_VARS))
				{
					++it;

                    if (it != end)
					{
						char d = *it;

                        if (d >= '0' && d <= '9')
						{
							int c = d - '0';

                            if (c < rc)
							{
								int o = ovec[c * 2];
								int l = ovec[c * 2 + 1] - o;
								result.append(subject, o, l);
							}
						}
						else
						{
							result += '$';
							result += d;
						}

                        ++it;
					}
					else
                        result += '$';
				}
				else
                    result += *it++;
			}

            pos = ovec[1];
			rp = result.length();
		}
		else
            result += subject[pos++];
	}

    subject = result;
	return rp;
}

bool Warhead::RegularExpression::match(std::string_view subject, std::string_view pattern, int options)
{
	int ctorOptions = options & (RE_CASELESS | RE_MULTILINE | RE_DOTALL | RE_EXTENDED | RE_ANCHORED | RE_DOLLAR_ENDONLY | RE_EXTRA | RE_UNGREEDY | RE_UTF8 | RE_NO_AUTO_CAPTURE);
	int mtchOptions = options & (RE_ANCHORED | RE_NOTBOL | RE_NOTEOL | RE_NOTEMPTY | RE_NO_AUTO_CAPTURE | RE_NO_UTF8_CHECK);
	RegularExpression re(pattern, ctorOptions, false);
	return re.match(subject, 0, mtchOptions);
}

int Warhead::RegularExpression::compileOptions(int options) const
{
	int pcreOptions = 0;

	if (options & RE_CASELESS)
		pcreOptions |= PCRE2_CASELESS;

    if (options & RE_MULTILINE)
		pcreOptions |= PCRE2_MULTILINE;

    if (options & RE_DOTALL)
		pcreOptions |= PCRE2_DOTALL;

    if (options & RE_EXTENDED)
		pcreOptions |= PCRE2_EXTENDED;

    if (options & RE_ANCHORED)
		pcreOptions |= PCRE2_ANCHORED;

    if (options & RE_DOLLAR_ENDONLY)
		pcreOptions |= PCRE2_DOLLAR_ENDONLY;

    if (options & RE_UNGREEDY)
		pcreOptions |= PCRE2_UNGREEDY;

    if (options & RE_UTF8)
		pcreOptions |= PCRE2_UTF | PCRE2_UCP;

    if (options & RE_NO_AUTO_CAPTURE)
		pcreOptions |= PCRE2_NO_AUTO_CAPTURE;

    if (options & RE_FIRSTLINE)
		pcreOptions |= PCRE2_FIRSTLINE;

    if (options & RE_DUPNAMES)
		pcreOptions |= PCRE2_DUPNAMES;

	return pcreOptions;
}

int Warhead::RegularExpression::matchOptions(int options) const
{
	int pcreOptions = 0;

	if (options & RE_ANCHORED)
		pcreOptions |= PCRE2_ANCHORED;

    if (options & RE_NOTBOL)
		pcreOptions |= PCRE2_NOTBOL;

    if (options & RE_NOTEOL)
		pcreOptions |= PCRE2_NOTEOL;

    if (options & RE_NOTEMPTY)
		pcreOptions |= PCRE2_NOTEMPTY;

    if (options & RE_NO_AUTO_CAPTURE)
		pcreOptions |= PCRE2_NO_AUTO_CAPTURE;

    if (options & RE_NO_UTF8_CHECK)
		pcreOptions |= PCRE2_NO_UTF_CHECK;

	return pcreOptions;
}
