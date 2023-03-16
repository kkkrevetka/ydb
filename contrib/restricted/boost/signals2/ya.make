# Generated by devtools/yamaker from nixpkgs 22.05.

LIBRARY()

LICENSE(BSL-1.0)

LICENSE_TEXTS(.yandex_meta/licenses.list.txt)

OWNER(
    g:cpp-contrib
    g:taxi-common
)

VERSION(1.81.0)

ORIGINAL_SOURCE(https://github.com/boostorg/signals2/archive/boost-1.81.0.tar.gz)

PEERDIR(
    contrib/restricted/boost/assert
    contrib/restricted/boost/bind
    contrib/restricted/boost/config
    contrib/restricted/boost/core
    contrib/restricted/boost/function
    contrib/restricted/boost/iterator
    contrib/restricted/boost/mpl
    contrib/restricted/boost/optional
    contrib/restricted/boost/parameter
    contrib/restricted/boost/predef
    contrib/restricted/boost/preprocessor
    contrib/restricted/boost/smart_ptr
    contrib/restricted/boost/throw_exception
    contrib/restricted/boost/tuple
    contrib/restricted/boost/type_traits
    contrib/restricted/boost/variant
)

ADDINCL(
    GLOBAL contrib/restricted/boost/signals2/include
)

NO_COMPILER_WARNINGS()

NO_UTIL()

END()