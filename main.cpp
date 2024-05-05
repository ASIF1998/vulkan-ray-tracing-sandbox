#include <hello_triangle/hello_triangle.hpp>
#include <junk_shop/junk_shop.hpp>

#include <base/logger/logger.hpp>

enum class Samples 
{
    HelloTriangle,
    JunkShop
};

int main(int argc, char* argv[])
{
    constexpr auto sample = Samples::JunkShop;

    try
    {
        std::unique_ptr<RayTracingBase> pApp;

        if constexpr (sample == Samples::HelloTriangle)
            pApp = std::make_unique<HelloTriangle>();
        else if constexpr (sample == Samples::JunkShop)
            pApp = std::make_unique<JunkShop>();

        pApp->init();
        pApp->show();
    }
    catch(const std::exception& ex)
    {
        log::appError("Msg from excpetion: {}", ex.what());
    }
    
    return 0;
}