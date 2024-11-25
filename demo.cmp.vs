/*
 * Определяем размер рабочей группы (work group size).
 *
 * Делая так, мы даём vulkan'у понять, что хотим, что бы
 * вычислительные шейдер выполнялся группами по 256 элементов
 * за раз.
 *
 * Рабочая группа может выполняться на одном из ядер, но это
 * на усмотрение драйвера.
 *
 * Обычно размер рабочей группы кратен 64 и не очень большой,
 * так как большой размер может требовать больше памяти и 
 * и может быть разделён на несколько ядер.
 *
*/
layout(local_size_x = 256) in;

layout(set = 0, binding = 0) uniform config
{
    mat4 transform;
    int matrix_count;
} op_data;

layout(set = 0, binding = 1) readonly buffer InputBuffer
{
    mat4 matrices[];
} source_data;

layout(set = 0, binding = 2) buffer OutputBuffer
{
    mat4 matrices[];
} output_data;

void main()
{
    uint gId = gl_GlobalInvocationID.x;

    if (gId < op_data.matrix_count)
        output_data.matrices[gId] = source_data.matrices[gId] * op_data.transform;
}