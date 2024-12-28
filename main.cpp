#include <hello_triangle/hello_triangle.hpp>
#include <junk_shop/junk_shop.hpp>
#include <dancing_penguin/dancing_penguin.hpp>

#include <base/math.hpp>

enum class Samples 
{
    HelloTriangle,
    JunkShop,
    DancingPenguin
};

int main(int argc, char* argv[])
{
    constexpr auto sample = Samples::DancingPenguin;

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
            case Samples::DancingPenguin:
                pApp = std::make_unique<DancingPenguin>();
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