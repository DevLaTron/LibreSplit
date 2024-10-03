#ifndef ASSERT_MACRO_H
#define ASSERT_MACRO_H
#define assertEqual( ... )               \
do {                                            \
if( !( __VA_ARGS__ ) ) {                     \
std::cerr << "Unit test assert [ " \
<< ( #__VA_ARGS__ )             \
<< " ] failed in line [ "       \
<< __LINE__                     \
<< " ] file [ "                 \
<< __FILE__ << " ]"             \
<< std::endl;                     \
err_code = 1;                           \
}                                            \
} while( false )
#endif //ASSERT_MACRO_H
