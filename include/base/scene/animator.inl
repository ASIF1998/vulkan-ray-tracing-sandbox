namespace sample_vk
{
    template<typename Keys>
    size_t AnimationSampler::getIndex(const Keys& keys, float time) const
    {
        auto begin = std::begin(keys);
        auto end = std::end(keys);

        auto it = std::lower_bound(begin, end, time, [] (const auto& key, auto time)
        {
            return key.time_stamp < time;
        });

        return std::distance(begin, it);
    }
}