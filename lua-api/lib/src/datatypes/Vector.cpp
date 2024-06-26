#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/vector_angle.hpp>

#include <datatypes/Vector.hpp>

namespace lua::datatypes {
    void bind_vectors(sol::state_view& lua) {
        #define BIND_VECTOR3_LIKE(name, datatype) \
            lua.new_usertype<name>(#name, \
                "clone", [](name& v) -> name { return v; }, \
                "x", &name::x, \
                "y", &name::y, \
                "z", &name::z, \
                "dot", [](name& v1, name& v2) { return glm::dot(v1, v2); }, \
                "cross", [](name& v1, name& v2) { return glm::cross(v1, v2); }, \
                "length", [](name& v) { return glm::length(v); }, \
                "normalize", [](name& v) { v = glm::normalize(v); }, \
                "normalized", [](name& v) { return glm::normalize(v); }, \
                "reflect", [](name& v, name& normal) { return glm::reflect(v, normal); }, \
                "refract", [](name& v, name& normal, datatype eta) { return glm::refract(v, normal, eta); }, \
                "lerp", [](name& v1, name& v2, datatype t) { return glm::lerp(v1, v2, t); }, \
                sol::meta_function::addition, [](name& lhs, name& rhs) { return lhs + rhs; }, \
                sol::meta_function::subtraction, [](name& lhs, name& rhs) { return lhs - rhs; }, \
                sol::meta_function::multiplication, [](name& lhs, datatype scalar) { return lhs * scalar; }
        
        #define BIND_VECTOR3_LIKE_END() \
            );

        BIND_VECTOR3_LIKE(Vector3f, float),
            sol::meta_function::construct, sol::constructors<Vector3f(float, float, float)>()
        BIND_VECTOR3_LIKE_END();

        BIND_VECTOR3_LIKE(Vector3d, double),
            sol::meta_function::construct, sol::constructors<Vector3d(double, double, double)>()
        BIND_VECTOR3_LIKE_END();
    }
}