//  polish_node.hpp
//  sa

#ifndef polish_node_hpp
#define polish_node_hpp

#include <cstdint>

namespace polish {

    class polish_node {
    public:
        using area_type = std::int32_t;
        using dimension_type = std::int32_t;    // YAL都是整型, 故而作此改动
        using size_type = std::uint32_t;

        static constexpr char VERTICAL_COMBINE = '+';
        static constexpr char HORIZONTAL_COMBINE = '*';
        static constexpr char NO_COMBINE = 'n'; // 实际并没有用

        char combine_type;	//*(左右结合) or +(上下结合)
        dimension_type height;
        dimension_type width;
        //bool counted;     // 应该用不到, 见polish_tree.hpp
        area_type area;
        polish_node* lc;	//left child
        polish_node* rc;	//right child
        polish_node* parent;	//parent node
        size_type size;     //subtree size

        polish_node(char combine_type_in, dimension_type height_in, 
            dimension_type width_in);

        void count_area();
    };

}

#endif /* polish_node_hpp */
