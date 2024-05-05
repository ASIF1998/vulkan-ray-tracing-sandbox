# Vulkan ray tracing sandbox
Это мой хобби-проект, над которым я работаю в свободное время. Основная цель - изучить и попрактиковаться в аппаратной трассировке лучей с использованием Vulkan API. 

**Не следует воспринимать этот проект как пример лучшего или наиболее производительного способа работы с трассировкой лучей на GPU: местами я делал то, что было быстрее и/или проще всего реализовать!**

# Компиляторы
Тестировал только на MSVC 19.38.33133.0.

# Зависимости
 - [Vulkan SDK](https://www.lunarg.com/vulkan-sdk/)
 - [glslang](https://github.com/KhronosGroup/glslang)
 - [assimp](https://github.com/assimp/assimp)
 - [STB](https://github.com/nothings/stb)
 - [SDL](https://github.com/libsdl-org/SDL)
 - [GLM](https://github.com/g-truc/glm) 

# Сцены

## HelloTriangle
Простой пример, где при помощи трассировки лучей рисуется треугольник.

### Результат
![hello-triangle](https://github.com/ASIF1998/vulkan-ray-tracing-sandbox/tree/main/results/hello-triangle/hello-triangle.png)

## JunkShop
В этой сцене рисуется модель [junkshop](https://www.aendom.com/tuts/junkshop-workflow-en/) при помощи Path tracing'а и [PBR](https://www.shadertoy.com/view/cll3R4). 

Чтобы переместить камеру, нажимайте клавиши W/S/A/D/SPACE/SHIFT. Камеру также можно повернуть, щелкнув кнопкой мыши по окну и перетащив мышь. При движении камеры будет присутствовать шум, т.к. рендер снова начнёт накапливать образцы.

### Результаты
![junk-shop-1](https://github.com/ASIF1998/vulkan-ray-tracing-sandbox/tree/main/results/junk-shop/junk-shop-1.png)
![junk-shop-2](https://github.com/ASIF1998/vulkan-ray-tracing-sandbox/tree/main/results/junk-shop/junk-shop-2.png)
![junk-shop-video](https://github.com/ASIF1998/vulkan-ray-tracing-sandbox/tree/main/results/junk-shop/junk-shop.mp4)