/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#ifndef BX_FILEPATH_H_HEADER_GUARD
#	error "Must be included from bx/filepath.h!"
#endif // BX_FILEPATH_H_HEADER_GUARD

namespace bx
{
	template<typename Ty>
	inline EnableIfType<isSame<Ty, FilePath>(), bool> isEqual(const Ty& _lhs, const Ty& _rhs, bool _caseSensitive)
	{
		return isEqual(StringView(_lhs), StringView(_rhs), _caseSensitive);
	}

} // namespace bx
