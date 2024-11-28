#include <hello_triangle/hello_triangle.hpp>
#include <junk_shop/junk_shop.hpp>
#include <animation/animation.hpp>

#include <base/math.hpp>

enum class Samples 
{
    HelloTriangle,
    JunkShop,
    Animation
};

int main(int argc, char* argv[])
{
    constexpr auto sample = Samples::Animation;

    try
    {
        std::unique_ptr<RayTracingBase> pApp;

        switch (sample)
        {
            case Samples::HelloTriangle:
                pApp = std::make_unique<HelloTriangle>();
                break;
            case Samples::JunkShop:
                pApp = std::make_unique<JunkShop>();
                break;
            case Samples::Animation:
                pApp = std::make_unique<Animation>();
                break;
        }

        pApp->init();
        pApp->show();
    }
    catch (...)
    {
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}