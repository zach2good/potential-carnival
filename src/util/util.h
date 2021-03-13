namespace util
{
    std::vector<char> uint32_to_vec(uint32_t n)
    {
        char a = (n & 0xFF000000) >> 24;
        char b = (n & 0x00FF0000) >> 16;
        char c = (n & 0x0000FF00) >> 8;
        char d = (n & 0x000000FF) >> 0;
        return { a, b, c, d };
    }

    uint32_t vec_to_uint32(std::vector<char> v)
    {
        char a = v[0] << 24;
        char b = v[0] << 16;
        char c = v[0] << 8;
        char d = v[0] << 0;
        return a + b + c + d;
    }

    void append_vecs(std::vector<char>& a, std::vector<char>& b)
    {
        a.insert(a.end(), b.begin(), b.end());
    }
}
