#include <cmath>
#include <cstdlib>
#include <fstream>
#include <print>
#include <string>
#include <vector>

#include "util.h"

struct Ray
{
    v3 origin;
    v3 direction;

    inline v3 at_param(float t) const { return origin + t * direction; }
};

struct Sphere
{
    v3 center;
    float radius;
};

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
        p = 2.0f * v3{ float(drand48()), float(drand48()), float(drand48()) } - v3{ 1.0f, 1.0f, 1.0f };
    }
    while (v3_dot(p, p) >= 1.0f);
    return p;
}

RayHitResult ray_hit_sphere(const Ray &ray, const World &world)
{
    float smallest_t = MAXFLOAT;
    const Sphere *hit_sphere = nullptr;

    for (size_t i = 0; i < world.spheres.size(); i++)
    {
        const Sphere &sphere = world.spheres[i];
        v3 oc = ray.origin - sphere.center;
        float a = v3_dot(ray.direction, ray.direction);
        float b = 2.0f * v3_dot(oc, ray.direction);
        float c = v3_dot(oc, oc) - sphere.radius * sphere.radius;
        float discriminant = b * b - 4 * a * c;

        if (discriminant >= 0.0f)
        {
            float root = (-b - std::sqrt(discriminant)) / (2.0f * a);
            if (root > 0.0f && root < smallest_t)
            {
                smallest_t = root;
                hit_sphere = &world.spheres[i];
            }
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

v3 ray_color(const Ray &ray, const World &world)
{
    v3 result_color;

    RayHitResult ray_hit_result = ray_hit_sphere(ray, world);
    if (ray_hit_result.hit)
    {
        v3 target = ray_hit_result.point + ray_hit_result.normal + random_in_unit_sphere();
        // v3 sphere_normal = ray_hit_result.normal;
        // v3 surface_normal_color = 0.5f * (sphere_normal + v3{ 1.0f, 1.0f, 1.0f });
        Ray bounce_ray{ ray_hit_result.point, target - ray_hit_result.point };
        v3 color_after_bounce = 0.5f * ray_color(bounce_ray, world);
        result_color = color_after_bounce;
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

    v3 lower_left_corner{ -2.0f, -1.0f, -1.0f };
    v3 horizontal{ 4.0f, 0.0f, 0.0f };
    v3 vertical{ 0.0f, 2.0f, 0.0f };
    v3 origin{ 0.0f, 0.0f, 0.0f };

    World world{};
    world.spheres.push_back(Sphere{ { 0.0f, 0.0f, -1.0f }, 0.5f });
    world.spheres.push_back(Sphere{ { 0.0f, -100.5f, -1.0f }, 100.0f });

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

                v3 target = lower_left_corner + u * horizontal + v * vertical;
                Ray ray{ origin, target };

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
