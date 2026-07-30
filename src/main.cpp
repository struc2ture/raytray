#include <cmath>
#include <cstdlib>
#include <fstream>
#include <print>

#include "util.h"

float ray_hit_sphere(const Ray &ray, const v3 &center, float radius)
{
    v3 oc = ray.origin - center;
    float a = v3_dot(ray.direction, ray.direction);
    float b = 2.0f * v3_dot(oc, ray.direction);
    float c = v3_dot(oc, oc) - radius * radius;
    float discriminant = b * b - 4 * a * c;

    float result_t;
    if (discriminant < 0)
    {
        result_t = -1.0f; // ray did not hit sphere
    }
    else
    {
        result_t = (-b - std::sqrt(discriminant)) / (2.0f * a); // the smaller of the two roots
    }
    return result_t;
}

v3 ray_color(const Ray &ray)
{
    v3 sphere_origin{ 0.0f, 0.0f, -1.0f };
    float sphere_radius = 0.5f;

    float t = ray_hit_sphere(ray, sphere_origin, sphere_radius);
    if (t > 0.0f)
    {
        v3 sphere_normal = v3_normalize(ray.at_param(t) - sphere_origin);
        v3 surface_normal_color = 0.5f * (sphere_normal + v3{ 1.0f, 1.0f, 1.0f });
        return surface_normal_color;
    }
    else
    {
        v3 unit_dir = v3_normalize(ray.direction);
        float t = 0.5f * (unit_dir.y + 1.0);
        v3 sky_color_blend = v3_lerp(v3{ 1.0f, 1.0f, 1.0f }, v3{ 0.5f, 0.7f, 1.0f }, t);
        return sky_color_blend;
    }
}

int main()
{
    int width = 200;
    int height = 100;
    int samples_per_pixel = 100;

    v3 lower_left_corner{ -2.0f, -1.0f, -1.0f };
    v3 horizontal{ 4.0f, 0.0f, 0.0f };
    v3 vertical{ 0.0f, 2.0f, 0.0f };
    v3 origin{ 0.0f, 0.0f, 0.0f };

    std::ofstream file("out/out.ppm");

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

                color += ray_color(ray);
            }

            color /= float(samples_per_pixel);

            int ir = int(255.99f * color.x);
            int ig = int(255.99f * color.y);
            int ib = int(255.99f * color.z);
            
            file << ir << ' ' << ig << ' ' << ib << '\n';
        }
    }

    std::println("Hello world");
    return 0;
}
