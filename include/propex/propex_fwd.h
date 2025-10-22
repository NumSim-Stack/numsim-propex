#ifndef PROPEX_FWD_H
#define PROPEX_FWD_H

namespace numsim::propex {

/**
 * @brief Forward declaration of node for property_view.
 * @tparam T          Stored value type.
 * @tparam Ownership  Ownership policy template.
 */
template <class T, template <class> class Ownership>
class node;

}

#endif // PROPEX_FWD_H
