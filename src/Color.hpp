class Color
{
  private:
    uint8_t m_r;
    uint8_t m_g;
    uint8_t m_b;

  public:
    Color(uint8_t r, uint8_t g, uint8_t b)
    {
        m_r = r;
        m_g = g;
        m_b = b;
    }

    uint8_t r() { return m_r; };
    uint8_t g() { return m_g; };
    uint8_t b() { return m_b; };
};
