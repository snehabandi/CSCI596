#include <math.h>
#include <iostream>
#include <stdio.h>

#include "ray.h"
#include "material.h"
#include "objects.h"


float Sphere::get_radius() { return m_r; }
Material* Sphere::get_material() { return material; }

// Check if ray intersects with sphere. Returns ObjectIntersection data structure
ObjectIntersection Sphere::get_intersection(const Ray &ray) {
	// Solve t^2*d.d + 2*t*(o-p).d + (o-p).(o-p)-R^2 = 0
	bool hit = false;
	float distance = 0;
	glm::vec3 n = glm::vec3();

	glm::vec3 op = position-ray.origin;
	float t, eps=1e-4, b=glm::dot(op, ray.direction), det=b*b-glm::dot(op, op)+m_r*m_r;
	if (det<0) return ObjectIntersection(hit, distance, n, material); 
	else det=sqrt(det);
	distance = (t=b-det)>eps ? t : ((t=b+det)>eps ? t : 0);
	if (distance != 0) hit = true, 
		n = glm::normalize(((ray.origin + ray.direction * distance) - position));

	return ObjectIntersection(hit, distance, n, material);
}


Mesh::Mesh(glm::vec3 p_, const char* file_path, Material* m_, glm::mat3 t_) : Object(p_, m_, t_){

	position=p_, material=m_;

    std::vector<tinyobj::shape_t> m_shapes;
    std::vector<tinyobj::material_t> materialaterials;


    std::string mtlbasepath;
    std::string inputfile = file_path;
    unsigned long pos = inputfile.find_last_of("/");
    mtlbasepath = inputfile.substr(0, pos+1);

    printf("Loading %s...\n", file_path);
    // Attempt to load mesh
	std::string err = tinyobj::LoadObj(m_shapes, materialaterials, inputfile.c_str(), mtlbasepath.c_str());

	if (!err.empty()) {
		std::cerr << err << std::endl;
		exit(1);
	}
	printf(" - Generating k-d tree...\n\n");

    long shapes_size, indices_size, materials_size;
    shapes_size = m_shapes.size();
    materials_size = materialaterials.size();

    // Load materials/textures from obj
    // TODO: Only texture is loaded at the moment, need to implement material types and colours
    for (int i=0; i<materials_size; i++) {
        std::string texture_path = "";

        if (!materialaterials[i].diffuse_texname.empty()){
            if (materialaterials[i].diffuse_texname[0] == '/') texture_path = materialaterials[i].diffuse_texname;
            texture_path = mtlbasepath + materialaterials[i].diffuse_texname;
            materials.push_back( new DiffuseMaterial(false, glm::vec3(1,1,1), glm::vec3(), texture_path.c_str()) );
        }
        else {
            materials.push_back( new DiffuseMaterial(false, glm::vec3(1,1,1), glm::vec3()) );
        }

    }

    // Load triangles from obj
    for (int i = 0; i < shapes_size; i++) {
        indices_size = m_shapes[i].mesh.indices.size() / 3;
        for (auto f = (decltype(indices_size)) 0; f < indices_size; f++) {

            // Triangle vertex coordinates
            glm::vec3 v0_ = t_*glm::vec3(
                    m_shapes[i].mesh.positions[ m_shapes[i].mesh.indices[3*f] * 3     ],
                    m_shapes[i].mesh.positions[ m_shapes[i].mesh.indices[3*f] * 3 + 1 ],
                    m_shapes[i].mesh.positions[ m_shapes[i].mesh.indices[3*f] * 3 + 2 ]
            ) + position;

            glm::vec3 v1_ = t_*glm::vec3(
                    m_shapes[i].mesh.positions[ m_shapes[i].mesh.indices[3*f + 1] * 3     ],
                    m_shapes[i].mesh.positions[ m_shapes[i].mesh.indices[3*f + 1] * 3 + 1 ],
                    m_shapes[i].mesh.positions[ m_shapes[i].mesh.indices[3*f + 1] * 3 + 2 ]
            )+ position;

            glm::vec3 v2_ = t_*glm::vec3(
                    m_shapes[i].mesh.positions[ m_shapes[i].mesh.indices[3*f + 2] * 3     ],
                    m_shapes[i].mesh.positions[ m_shapes[i].mesh.indices[3*f + 2] * 3 + 1 ],
                    m_shapes[i].mesh.positions[ m_shapes[i].mesh.indices[3*f + 2] * 3 + 2 ]
        )+ position;

            glm::vec3 t0_, t1_, t2_;

            //Attempt to load triangle texture coordinates
            if (m_shapes[i].mesh.indices[3 * f + 2] * 2 + 1 < m_shapes[i].mesh.texcoords.size()) {
                t0_ = glm::vec3(
                        m_shapes[i].mesh.texcoords[m_shapes[i].mesh.indices[3 * f] * 2],
                        m_shapes[i].mesh.texcoords[m_shapes[i].mesh.indices[3 * f] * 2 + 1],
                        0
                );

                t1_ = glm::vec3(
                        m_shapes[i].mesh.texcoords[m_shapes[i].mesh.indices[3 * f + 1] * 2],
                        m_shapes[i].mesh.texcoords[m_shapes[i].mesh.indices[3 * f + 1] * 2 + 1],
                        0
                );

                t2_ = glm::vec3(
                        m_shapes[i].mesh.texcoords[m_shapes[i].mesh.indices[3 * f + 2] * 2],
                        m_shapes[i].mesh.texcoords[m_shapes[i].mesh.indices[3 * f + 2] * 2 + 1],
                        0
                );
            }
            else {
                t0_=glm::vec3();
                t1_=glm::vec3();
                t2_=glm::vec3();
            }

            if (m_shapes[i].mesh.material_ids[ f ] != -1 &&  m_shapes[i].mesh.material_ids[ f ] < materials.size())
                tris.push_back(new Triangle(v0_, v1_, v2_, t0_, t1_, t2_, materials[ m_shapes[i].mesh.material_ids[ f ] ]));
            else
                tris.push_back(new Triangle(v0_, v1_, v2_, t0_, t1_, t2_, material));
        }
    }

    node = KDNode().build(tris, 0);
    printf("\n");
	//bvh = BVH(&tris);
}

const Material *dptr;
// Check if ray intersects with mesh. Returns ObjectIntersection data structure
ObjectIntersection Mesh::get_intersection(const Ray &ray) {
    float t=0, tmin=INFINITY;
    glm::vec3 normal = glm::vec3();
    glm::vec3 colour = glm::vec3();
    bool hit = node->hit(node, ray, t, tmin, normal, colour);
    //bool hit = bvh.getIntersection(ray, t, tmin, normal);
    return ObjectIntersection(hit, tmin, normal, new DiffuseMaterial(false, colour, glm::vec3(), Texture(), true));

}