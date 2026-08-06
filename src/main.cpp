// TODO: I think there's a possibility of an infinite loop when refracting into a volume. The transmitted ray might still be considered originating from outside the volume, refracting into the volume again.

#include <cmath>
#include <cstdlib>
#include <fstream>
#include <print>
#include <string>
#include <vector>

#include "util.h"

constexpr int MAX_BOUNCE_DEPTH = 50;

struct Ray
{
    v3 origin;
    v3 direction;

    inline v3 at_param(float t) const { return origin + t * direction; }
};

struct Sphere
{
    enum class Material
    {
        Lambertian,
        Metal,
        Dielectric
    };

    v3 center;
    float radius;
    Material material;
    v3 albedo;
    float fuzz;
    float refractive_index;
};

Sphere make_sphere_lambertian(v3 center, float radius, v3 albedo)
{
    Sphere sphere{};
    sphere.center = center;
    sphere.radius = radius;
    sphere.material = Sphere::Material::Lambertian;
    sphere.albedo = albedo;
    return sphere;
}

Sphere make_sphere_metal(v3 center, float radius, v3 albedo, float fuzz = 0.0f)
{
    Sphere sphere{};
    sphere.center = center;
    sphere.radius = radius;
    sphere.material = Sphere::Material::Metal;
    sphere.albedo = albedo;
    sphere.fuzz = fuzz;
    return sphere;
}

Sphere make_sphere_dielectric(v3 center, float radius, float refractive_index)
{
    Sphere sphere{};
    sphere.center = center;
    sphere.radius = radius;
    sphere.material = Sphere::Material::Dielectric;
    sphere.refractive_index = refractive_index;
    return sphere;
}

struct World
{
    std::vector<Sphere> spheres;
};

struct RayHitResult
{
    bool hit;
    v3 point;
    v3 normal;
    const Sphere *sphere;
};

v3 random_in_unit_sphere()
{
    v3 p;
    do
    {
        p = 2.0f * v3{float(drand48()), float(drand48()), float(drand48())} - v3{1.0f, 1.0f, 1.0f};
    }
    while (v3_dot(p, p) >= 1.0f);
    return p;
}

v3 random_in_unit_disk()
{
    v3 p;
    do
    {
        p = 2.0f * v3{float(drand48()), float(drand48()), 0.0f} - v3{1.0f, 1.0f, 0.0f};
    }
    while (v3_dot(p, p) >= 1.0f);
    return p;
}

struct Camera
{
    v3 origin;
    v3 lower_left_corner;
    v3 horizontal;
    v3 vertical;
    v3 u, v, w;
    float lens_radius;

    inline void init(v3 eye, v3 look_at, v3 view_up, float v_fov, float aspect_wh, float aperture = 0.0f, float focus_dist = 1.0f)
    {
        float theta = v_fov * M_PI / 180.0f;
        float half_height = tan(theta * 0.5f);
        float half_width = aspect_wh * half_height;

        // (u, v, w) basis
        w = v3_normalize(eye - look_at);
        u = v3_normalize(v3_cross(view_up, w));
        v = v3_normalize(v3_cross(w, u));

        origin = eye;
        lower_left_corner = origin - u * half_width * focus_dist - v * half_height * focus_dist - w * focus_dist;
        horizontal = 2.0f * half_width * focus_dist * u;
        vertical = 2.0f * half_height * focus_dist * v;

        lens_radius = aperture * 0.5f;
    }

    inline Ray get_ray(float s, float t)
    {
        Ray result;

        if (lens_radius > 0.0f)
        {
            v3 random_offset_local = lens_radius * random_in_unit_disk();
            v3 random_offset_global = random_offset_local.x * u + random_offset_local.y * v;

            result.origin = origin + random_offset_global;
            result.direction = lower_left_corner + s * horizontal + t * vertical - result.origin;
        }
        else
        {
            result.origin = origin;
            result.direction = lower_left_corner + s * horizontal + t * vertical - result.origin;
        }

        return result;
    }
};

v3 reflect(const v3 &vec, const v3 &normal)
{
    v3 result = vec - 2.0f * v3_dot(vec, normal) * normal;
    return result;
}

bool refract(const v3 &vec, const v3 &normal, float ni_over_nt, v3 &out_refracted)
{
    v3 dir = v3_normalize(vec);
    float dt = v3_dot(dir, normal);
    float discriminant = 1.0f - ni_over_nt * ni_over_nt * (1.0f - dt * dt);
    if (discriminant > 0.0f)
    {
        out_refracted = ni_over_nt *(vec - normal * dt) - normal * sqrt(discriminant);
        return true;
    }
    else
    {
        return false;
    }
}

float shlick(float cosine, float refractive_index)
{
    float result;

    float r0 = (1 - refractive_index) / (1 + refractive_index);
    r0 *= r0;
    result = r0 + (1 - r0) * pow(1 - cosine, 5);

    return result;
}

bool ray_hit_sphere(const Ray &ray, const Sphere &sphere, float t_min, float t_max, float &out_root)
{
    bool solved;

    v3 oc = ray.origin - sphere.center;
    float a = v3_dot(ray.direction, ray.direction);
    float b = v3_dot(oc, ray.direction);
    float c = v3_dot(oc, oc) - sphere.radius * sphere.radius;
    float discriminant = b * b - a * c;
    if (discriminant > 0.0f)
    {
        bool found_root_in_range = false;

        float root = (-b - sqrt(discriminant)) / a;
        if (root > t_min && root < t_max)
        {
            found_root_in_range = true;
        }

        if (!found_root_in_range)
        {
            root = (-b + sqrt(discriminant)) / a;
            if (root > t_min && root < t_max)
            {
                found_root_in_range = true;
            }
        }

        if (found_root_in_range)
        {
            out_root = root;
        }
        solved = found_root_in_range;
    }
    else
    {
        solved = false;
    }

    return solved;
}

RayHitResult ray_closest_hit(const Ray &ray, const World &world)
{
    const Sphere *hit_sphere = nullptr;
    float smallest_t = MAXFLOAT;

    for (size_t i = 0; i < world.spheres.size(); i++)
    {
        const Sphere &sphere = world.spheres[i];

        float t;
        if (ray_hit_sphere(ray, sphere, 0.01f, smallest_t, t))
        {
            smallest_t = t;
            hit_sphere = &world.spheres[i];
        }
    }

    RayHitResult result;
    
    if (hit_sphere)
    {
        result.hit = true;
        result.sphere = hit_sphere;
        result.point = ray.at_param(smallest_t);
        result.normal = (result.point - hit_sphere->center) / hit_sphere->radius;
    }
    else
    {
        result.hit = false;
    }

    return result;
}

bool ray_scatter(const Ray &ray, const RayHitResult &hit_result, Ray &out_scattered_ray, v3 &out_attenuation)
{
    switch (hit_result.sphere->material)
    {
        case Sphere::Material::Lambertian:
        {
            v3 target = hit_result.point + hit_result.normal + random_in_unit_sphere();
            out_scattered_ray = Ray{ hit_result.point, target - hit_result.point };
            out_attenuation = hit_result.sphere->albedo;
            return true;
        } break;

        case Sphere::Material::Metal:
        {
            v3 reflected_dir = reflect(v3_normalize(ray.direction), hit_result.normal);
            out_scattered_ray = Ray{ hit_result.point, reflected_dir + hit_result.sphere->fuzz * random_in_unit_sphere() };
            out_attenuation = hit_result.sphere->albedo;
            bool scattered_outside_sphere = v3_dot(out_scattered_ray.direction, hit_result.normal) > 0.0f;
            return scattered_outside_sphere;
        } break;

        case Sphere::Material::Dielectric:
        {
            v3 ray_dir = v3_normalize(ray.direction);
            out_attenuation = v3{ 1.0f, 1.0f, 1.0f };

            // this code is limited to the case of one of the sides of the refraction surface being air (n = 1)
            v3 outward_normal;
            float ni_over_nt; // refractive index: incident over transmitted
            float cosine;
            float dot_product = v3_dot(ray_dir, hit_result.normal);
            if (dot_product > 0.0f)
            {
                // the ray originated inside a volume, the "outward" normal will point against the ray -> back inside the volume
                outward_normal = -hit_result.normal;
                ni_over_nt = hit_result.sphere->refractive_index; // inside the volume is incident, the air is transmitted -> ni_over_nt = ni / 1.0f
                cosine = hit_result.sphere->refractive_index * dot_product / ray.direction.length();
            }
            else
            {
                outward_normal = hit_result.normal;
                ni_over_nt = 1.0f / hit_result.sphere->refractive_index;
                cosine = -dot_product / ray.direction.length();
            }

            bool refracted = false;
            v3 refracted_dir;
            if (refract(ray_dir, outward_normal, ni_over_nt, refracted_dir))
            {
                float reflect_probability = shlick(cosine, hit_result.sphere->refractive_index);
                if (drand48() >= reflect_probability)
                {
                    out_scattered_ray = Ray{ hit_result.point, refracted_dir };
                    refracted = true;
                }
            }

            if (!refracted)
            {
                // TODO: shouldn't this also use outward_normal? Or do we assume no reflections when inside a dielectric?
                v3 reflected_dir = reflect(ray_dir, hit_result.normal);
                out_scattered_ray = Ray{ hit_result.point, reflected_dir };
            }
            
            // never absorbed
            return true;
        } break;
    }
}

v3 ray_color(const Ray &ray, const World &world, int depth = 0)
{
    v3 result_color;

    RayHitResult ray_hit_result = ray_closest_hit(ray, world);
    if (ray_hit_result.hit)
    {
        Ray scattered_ray;
        v3 attenuation;
        if (depth < MAX_BOUNCE_DEPTH && ray_scatter(ray, ray_hit_result, scattered_ray, attenuation))
        {
            v3 color_after_bounce = attenuation * ray_color(scattered_ray, world, depth + 1);
            result_color = color_after_bounce;
        }
        else
        {
            result_color = v3{ 0.0f, 0.0f, 0.0f };
        }
    }
    else
    {
        v3 unit_dir = v3_normalize(ray.direction);
        float t = 0.5f * (unit_dir.y + 1.0);
        v3 sky_color_blend = v3_lerp(v3{ 1.0f, 1.0f, 1.0f }, v3{ 0.5f, 0.7f, 1.0f }, t);
        result_color = sky_color_blend;
    }

    return result_color;
}

int main()
{
    std::println("Starting ray tracing...");

    int width = 200;
    int height = 100;
    int samples_per_pixel = 100;

    Camera camera;
    v3 camera_pos = v3{-2.0f, 2.0f, 1.0f};
    v3 camera_look_at = v3{0.0f, 0.0f, -1.0f};
    float focus_dist = (camera_look_at - camera_pos).length();
    camera.init(camera_pos, camera_look_at, v3{0.0f, 1.0f, 0.0f}, 30.0f, float(width) / float(height), 0.2f, focus_dist);
    // camera.init(camera_pos, camera_look_at, v3{0.0f, 1.0f, 0.0f}, 30.0f, float(width) / float(height), 0.0f, 1.0f);

    World world{};
    world.spheres.push_back(make_sphere_lambertian(v3{ 0.0f, 0.0f, -1.0f }, 0.5f, v3{ 0.1f, 0.2f, 0.5f }));
    world.spheres.push_back(make_sphere_lambertian(v3{ 0.0f, -100.5f, -1.0f }, 100.0f, v3{ 0.8f, 0.8f, 0.0f }));
    world.spheres.push_back(make_sphere_metal(v3{ 1.0f, 0.0f, -1.0f }, 0.5f, v3{ 0.8f, 0.6f, 0.2f }));
    world.spheres.push_back(make_sphere_dielectric(v3{ -1.0f, 0.0f, -1.0f }, 0.5f, 1.50f));
    world.spheres.push_back(make_sphere_dielectric(v3{ -1.0f, 0.0f, -1.0f }, -0.45f, 1.50f));

    std::string out_name("out/out.ppm");

    std::ofstream file(out_name);

    file << "P3\n";
    file << width << ' ' << height << '\n';
    file << "255\n";

    for (int y = height - 1; y >= 0; y--)
    {
        for (int x = 0; x < width; x++)
        {
            v3 color{ 0.0f, 0.0f, 0.0f };

            for (int sample_i = 0; sample_i < samples_per_pixel; sample_i++)
            {
                float u = float(x + drand48()) / float(width);
                float v = float(y + drand48()) / float(height);

                Ray ray = camera.get_ray(u, v);

                color += ray_color(ray, world);
            }

            color /= float(samples_per_pixel);

            // gamma corrected (sort of)
            color = v3{ std::sqrt(color.x), std::sqrt(color.y), std::sqrt(color.z) };

            int ir = int(255.99f * color.x);
            int ig = int(255.99f * color.y);
            int ib = int(255.99f * color.z);
            
            file << ir << ' ' << ig << ' ' << ib << '\n';
        }
    }

    std::println("Finished. Result written to {}", out_name);
    return 0;
}
