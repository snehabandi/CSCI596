#include <vector>

#include "ray.h"
#include "kdtree.h"

// Build KD tree for tris
KDNode* KDNode::build(std::vector<Triangle*> &tris, int depth){
    KDNode* node = new KDNode();
    node->leaf = false;
    node->triangles = std::vector<Triangle*>();
    node->left = NULL;
    node->right = NULL;
    node->box = AABB();

    if (tris.size() == 0) return node;

    if (depth > 25 || tris.size() <= 6) {
        node->triangles = tris;
        node->leaf = true;
        node->box = tris[0]->get_bounding_box();

        for (size_t i=1; i<tris.size(); i++) {
            node->box.expand(tris[i]->get_bounding_box());
        }

        node->left = new KDNode();
        node->right = new KDNode();
        node->left->triangles = std::vector<Triangle*>();
        node->right->triangles = std::vector<Triangle*>();

        return node;
    }

    node->box = tris[0]->get_bounding_box();
    glm::vec3 avg_midpt = glm::vec3();
    float tris_recp = 1.0/tris.size();

    for (size_t i=1; i<tris.size(); i++) {
        node->box.expand(tris[i]->get_bounding_box());
        avg_midpt = avg_midpt + (tris[i]->get_midpoint() * tris_recp);
    }

    // For each axis, get mean of coordinate of midpoint of triangle
    // Then take the axis with the biggest span & distribute the triangle into left & right of the tree based
    // on whether it is bigger than average or not


    std::vector<Triangle*> left_tris;
    std::vector<Triangle*> right_tris;
    int axis = node->box.get_longest_axis();

    for(auto triangle: tris){
        if(avg_midpt[axis] >= triangle->get_midpoint()[axis]){
            left_tris.push_back(triangle);
        }
        else{
            right_tris.push_back(triangle);
        }
    }

    
    if (tris.size() == left_tris.size() || tris.size() == right_tris.size()) {
        node->triangles = tris;
        node->leaf = true;
        node->box = tris[0]->get_bounding_box();

        for (size_t i=1; i<tris.size(); i++) {
            node->box.expand(tris[i]->get_bounding_box());
        }

        node->left = new KDNode();
        node->right = new KDNode();
        node->left->triangles = std::vector<Triangle*>();
        node->right->triangles = std::vector<Triangle*>();

        return node;
    }

    node->left = build(left_tris, depth+1);
    node->right = build(right_tris, depth+1);

    return node;
}

// Finds nearest triangle in kd tree that intersects with ray.
bool KDNode::hit(KDNode *node, const Ray &ray, float &t, float &tmin, glm::vec3 &normal, glm::vec3 &c) {
    float dist;
    if (node->box.intersection(ray, dist)){
        if (dist > tmin) return false;

        bool hit_tri = false;
        bool hit_left = false;
        bool hit_right = false;
        long tri_idx;

        if (!node->leaf) {
            //if ( node->left->triangles.size() > 0 )
                hit_left = hit(node->left, ray, t, tmin, normal, c);

            //if ( node->right->triangles.size() > 0 )
                hit_right = hit(node->right, ray, t, tmin, normal, c);

            return hit_left || hit_right;
        }
        else {
            size_t triangles_size = node->triangles.size();
            for (size_t i=0; i<triangles_size; i++) {
                if (node->triangles[i]->intersect(ray, t, tmin, normal)){
                    hit_tri = true;
                    tmin = t;
                    tri_idx = i;
                }
            }
            if (hit_tri) {
                glm::vec3 p = ray.origin + ray.direction * tmin;
                c = node->triangles[tri_idx]->get_colour_at(p);
                return true;
            }
        }
    }
    return false;
}