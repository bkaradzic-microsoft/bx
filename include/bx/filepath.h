/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#ifndef BX_FILEPATH_H_HEADER_GUARD
#define BX_FILEPATH_H_HEADER_GUARD

#include "error.h"
#include "string.h"

namespace bx
{
	BX_ERROR_RESULT(kErrorAccess,       BX_MAKEFOURCC('b', 'x', 1, 1) );
	BX_ERROR_RESULT(kErrorNotDirectory, BX_MAKEFOURCC('b', 'x', 1, 2) );

	constexpr int32_t kMaxFilePath = 1024;

	/// Special predefined OS directories.
	///
	struct Dir
	{
		/// Special OS directories:
		enum Enum
		{
			Current,    //!< Current directory.
			Executable, //!< Executable file path.
			Home,       //!< User's home directory.
			Temp,       //!< Temporary directory.

			Count
		};
	};

	/// FilePath parser and helper.
	///
	/// /abv/gd/555/333/pod.mac
	/// ppppppppppppppppbbbeeee
	/// ^               ^  ^
	/// +-path     base-+  +-ext
	///                 ^^^^^^^
	///                 +-filename
	///
	class FilePath
	{
	public:
		/// Default constructor, creates empty file path.
		///
		FilePath();

		/// Construct file path from special OS directory.
		///
		FilePath(Dir::Enum _dir);

		/// Construct file path from C string.
		///
		FilePath(const char* _str);

		/// Construct file path from string.
		///
		FilePath(const StringView& _str);

		/// Assign file path from string.
		///
		FilePath& operator=(const char* _rhs);

		/// Assign file path from string.
		///
		FilePath& operator=(const StringView& _rhs);

		/// Clear file path.
		///
		void clear();

		/// Set file path from special OS directory.
		///
		void set(Dir::Enum _dir);

		/// Set file path.
		///
		void set(const StringView& _str);

		/// Join directory to file path.
		///
		void join(const StringView& _str);

		/// Implicitly converts FilePath to StringView.
		///
		operator StringView() const;

		/// Returns zero-terminated C string pointer to file path.
		///
		const char* getCPtr() const;

		/// If path is `/abv/gd/555/333/pod.mac` returns `/abv/gd/555/333/`.
		///
		StringView getPath() const;

		/// If path is `/abv/gd/555/333/pod.mac` returns `pod.mac`.
		///
		StringView getFileName() const;

		/// If path is `/abv/gd/555/333/pod.mac` returns `pod`.
		///
		StringView getBaseName() const;

		/// If path is `/abv/gd/555/333/pod.mac` returns `.mac`.
		///
		StringView getExt() const;

		/// Returns true if file path is absolute.
		///
		bool isAbsolute() const;

		/// Returns true if file path is empty.
		///
		bool isEmpty() const;

	private:
		char m_filePath[kMaxFilePath];
	};

	/// True if file names are case sensitive by platform convention.
	///
	/// This reflects the platform convention rather than the file system actually in use,
	/// which on both Windows and macOS can be configured either way.
	///
	constexpr bool kFilePathCaseSensitive = !BX_PLATFORM_WINDOWS;

	/// Returns true if two file paths are equal.
	///
	/// Paths are compared as normalized by `FilePath`, so `a//b/../b/c` and `a/b/c` are
	/// equal. No file system is accessed, thus links and mount points that alias the same
	/// file are not resolved.
	///
	/// @param[in] _lhs Left-hand side file path.
	/// @param[in] _rhs Right-hand side file path.
	/// @param[in] _caseSensitive Use case sensitive comparison if true.
	///
	template<typename Ty>
	EnableIfType<isSame<Ty, FilePath>(), bool> isEqual(const Ty& _lhs, const Ty& _rhs, bool _caseSensitive = kFilePathCaseSensitive);

} // namespace bx

#include "inline/filepath.inl"

#endif // BX_FILEPATH_H_HEADER_GUARD
