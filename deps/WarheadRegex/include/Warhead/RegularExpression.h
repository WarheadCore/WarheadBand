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

#ifndef _WARHEAD_REGULAR_EXCEPTION_H_
#define _WARHEAD_REGULAR_EXCEPTION_H_

#include <map>
#include <vector>
#include <string>
#include <string_view>

namespace Warhead
{
    class RegularExpression
	    /// A class for working with regular expressions.
	    /// Implemented using PCRE2, the Perl Compatible
	    /// Regular Expressions library by Philip Hazel
	    /// (see http://www.pcre.org).
    {
    public:
	    enum Options
		    /// Some of the following options can only be passed to the constructor;
		    /// some can be passed only to matching functions, and some can be used
		    /// everywhere.
		    ///
		    ///   * Options marked [ctor] can be passed to the constructor.
		    ///   * Options marked [match] can be passed to match, extract, split and subst.
		    ///   * Options marked [subst] can be passed to subst.
		    ///
		    /// See the PCRE documentation for more information.
	    {
		    RE_CASELESS        = 0x00000001, /// case insensitive matching (/i) [ctor]
		    RE_MULTILINE       = 0x00000002, /// enable multi-line mode; affects ^ and $ (/m) [ctor]
		    RE_DOTALL          = 0x00000004, /// dot matches all characters, including newline (/s) [ctor]
		    RE_EXTENDED        = 0x00000008, /// totally ignore whitespace (/x) [ctor]
		    RE_ANCHORED        = 0x00000010, /// treat pattern as if it starts with a ^ [ctor, match]
		    RE_DOLLAR_ENDONLY  = 0x00000020, /// dollar matches end-of-string only, not last newline in string [ctor]
		    RE_EXTRA           = 0x00000040, /// enable optional PCRE functionality [ctor]
		    RE_NOTBOL          = 0x00000080, /// circumflex does not match beginning of string [match]
		    RE_NOTEOL          = 0x00000100, /// $ does not match end of string [match]
		    RE_UNGREEDY        = 0x00000200, /// make quantifiers ungreedy [ctor]
		    RE_NOTEMPTY        = 0x00000400, /// empty string never matches [match]
		    RE_UTF8            = 0x00000800, /// assume pattern and subject is UTF-8 encoded [ctor]
		    RE_NO_AUTO_CAPTURE = 0x00001000, /// disable numbered capturing parentheses [ctor, match]
		    RE_NO_UTF8_CHECK   = 0x00002000, /// do not check validity of UTF-8 code sequences [match]
		    RE_FIRSTLINE       = 0x00040000, /// an  unanchored  pattern  is  required  to  match
		                                     /// before  or  at  the  first  newline  in  the subject string,
		                                     /// though the matched text may continue over the newline [ctor]
		    RE_DUPNAMES        = 0x00080000, /// names used to identify capturing  subpatterns need not be unique [ctor]
		    RE_NEWLINE_CR      = 0x00100000, /// assume newline is CR ('\r'), the default [ctor]
		    RE_NEWLINE_LF      = 0x00200000, /// assume newline is LF ('\n') [ctor]
		    RE_NEWLINE_CRLF    = 0x00300000, /// assume newline is CRLF ("\r\n") [ctor]
		    RE_NEWLINE_ANY     = 0x00400000, /// assume newline is any valid Unicode newline character [ctor]
		    RE_NEWLINE_ANYCRLF = 0x00500000, /// assume newline is any of CR, LF, CRLF [ctor]
		    RE_GLOBAL          = 0x10000000, /// replace all occurences (/g) [subst]
		    RE_NO_VARS         = 0x20000000  /// treat dollar in replacement string as ordinary character [subst]
	    };

	    struct Match
	    {
		    std::string::size_type offset; /// zero based offset (std::string::npos if subexpr does not match)
		    std::string::size_type length; /// length of substring
		    std::string name;              /// name of group
	    };

        using MatchVec = std::vector<Match>;
	    using GroupMap = std::map<int, std::string>;

	    RegularExpression(std::string_view pattern, int options = 0, bool study = true);
		    /// Creates a regular expression and parses the given pattern.
		    /// Note: the study argument is only provided for backwards compatibility
		    /// and is ignored since POCO release 1.12, which uses PCRE2.
		    /// For a description of the options, please see the PCRE documentation.
		    /// Throws a RegularExpressionException if the patter cannot be compiled.

	    ~RegularExpression();
		    /// Destroys the regular expression.

	    int match(std::string_view subject, Match& match, int options = 0) const;
		    /// Matches the given subject string against the pattern. Returns the position
		    /// of the first captured substring in mtch.
		    /// If no part of the subject matches the pattern, mtch.offset is std::string::npos and
		    /// mtch.length is 0.
		    /// Throws a RegularExpressionException in case of an error.
		    /// Returns the number of matches.

	    int match(std::string_view subject, std::string::size_type offset, Match& match, int options = 0) const;
		    /// Matches the given subject string, starting at offset, against the pattern.
		    /// Returns the position of the captured substring in mtch.
		    /// If no part of the subject matches the pattern, mtch.offset is std::string::npos and
		    /// mtch.length is 0.
		    /// Throws a RegularExpressionException in case of an error.
		    /// Returns the number of matches.

	    int match(std::string_view subject, std::string::size_type offset, MatchVec& matches, int options = 0) const;
		    /// Matches the given subject string against the pattern.
		    /// The first entry in matches contains the position of the captured substring.
		    /// The following entries identify matching subpatterns. See the PCRE documentation
		    /// for a more detailed explanation.
		    /// If no part of the subject matches the pattern, matches is empty.
		    /// Throws a RegularExpressionException in case of an error.
		    /// Returns the number of matches.

	    bool match(std::string_view subject, std::string::size_type offset = 0) const;
		    /// Returns true if and only if the subject matches the regular expression.
		    ///
		    /// Internally, this method sets the RE_ANCHORED and RE_NOTEMPTY options for
		    /// matching, which means that the empty string will never match and
		    /// the pattern is treated as if it starts with a ^.

	    bool match(std::string_view subject, std::string::size_type offset, int options) const;
		    /// Returns true if and only if the subject matches the regular expression.

	    bool operator==(std::string_view subject) const;
		    /// Returns true if and only if the subject matches the regular expression.
		    ///
		    /// Internally, this method sets the RE_ANCHORED and RE_NOTEMPTY options for
		    /// matching, which means that the empty string will never match and
		    /// the pattern is treated as if it starts with a ^.

	    bool operator!=(std::string_view subject) const;
		    /// Returns true if and only if the subject does not match the regular expression.
		    ///
		    /// Internally, this method sets the RE_ANCHORED and RE_NOTEMPTY options for
		    /// matching, which means that the empty string will never match and
		    /// the pattern is treated as if it starts with a ^.

	    int extract(std::string_view subject, std::string& str, int options = 0) const;
		    /// Matches the given subject string against the pattern.
		    /// Returns the captured string.
		    /// Throws a RegularExpressionException in case of an error.
		    /// Returns the number of matches.

	    int extract(std::string_view subject, std::string::size_type offset, std::string& str, int options = 0) const;
		    /// Matches the given subject string, starting at offset, against the pattern.
		    /// Returns the captured string.
		    /// Throws a RegularExpressionException in case of an error.
		    /// Returns the number of matches.

	    int split(std::string_view subject, std::vector<std::string>& strings, int options = 0) const;
		    /// Matches the given subject string against the pattern.
		    /// The first entry in captured is the captured substring.
		    /// The following entries contain substrings matching subpatterns. See the PCRE documentation
		    /// for a more detailed explanation.
		    /// If no part of the subject matches the pattern, captured is empty.
		    /// Throws a RegularExpressionException in case of an error.
		    /// Returns the number of matches.

	    int split(std::string_view subject, std::string::size_type offset, std::vector<std::string>& strings, int options = 0) const;
		    /// Matches the given subject string against the pattern.
		    /// The first entry in captured is the captured substring.
		    /// The following entries contain substrings matching subpatterns. See the PCRE documentation
		    /// for a more detailed explanation.
		    /// If no part of the subject matches the pattern, captured is empty.
		    /// Throws a RegularExpressionException in case of an error.
		    /// Returns the number of matches.

	    int subst(std::string& subject, std::string_view replacement, int options = 0) const;
		    /// Substitute in subject all matches of the pattern with replacement.
		    /// If RE_GLOBAL is specified as option, all matches are replaced. Otherwise,
		    /// only the first match is replaced.
		    /// Occurrences of $<n> (for example, $1, $2, ...) in replacement are replaced
		    /// with the corresponding captured string. $0 is the original subject string.
		    /// Returns the number of replaced occurrences.

	    int subst(std::string& subject, std::string::size_type offset, std::string_view replacement, int options = 0) const;
		    /// Substitute in subject all matches of the pattern with replacement,
		    /// starting at offset.
		    /// If RE_GLOBAL is specified as option, all matches are replaced. Otherwise,
		    /// only the first match is replaced.
		    /// Unless RE_NO_VARS is specified, occurrences of $<n> (for example, $0, $1, $2, ... $9)
		    /// in replacement are replaced with the corresponding captured string.
		    /// $0 is the captured substring. $1 ... $n are the substrings matching the subpatterns.
		    /// Returns the number of replaced occurrences.

	    static bool match(std::string_view subject, std::string_view pattern, int options = 0);
		    /// Matches the given subject string against the regular expression given in pattern,
		    /// using the given options.

    private:
        std::string::size_type substOne(std::string& subject, std::string::size_type offset, std::string_view replacement, int options) const;
        int compileOptions(int options) const;
        int matchOptions(int options) const;

	    // Note: to avoid a dependency on the pcre2.h header the following are
	    // declared as void* and casted to the correct type in the implementation file.
	    void* _pcre{ nullptr };  // Actual type is pcre2_code_8*

	    GroupMap _groups;

	    RegularExpression() = delete;
	    RegularExpression(const RegularExpression&) = delete;
		RegularExpression(RegularExpression&&) = delete;
	    RegularExpression& operator=(const RegularExpression&) = delete;
	    RegularExpression& operator=(RegularExpression&&) = delete;
    };

    //
    // inlines
    //
    inline int RegularExpression::match(std::string_view subject, Match& mtch, int options) const
    {
	    return match(subject, 0, mtch, options);
    }

    inline int RegularExpression::split(std::string_view subject, std::vector<std::string>& strings, int options) const
    {
	    return split(subject, 0, strings, options);
    }

    inline int RegularExpression::subst(std::string& subject, std::string_view replacement, int options) const
    {
	    return subst(subject, 0, replacement, options);
    }

    inline bool RegularExpression::operator==(std::string_view subject) const
    {
	    return match(subject);
    }

    inline bool RegularExpression::operator!=(std::string_view subject) const
    {
	    return !match(subject);
    }
}

#endif // _WARHEAD_REGULAR_EXCEPTION_H_
