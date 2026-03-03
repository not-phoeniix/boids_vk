// implemented based on PLF library's conceptual overview:
//   https://www.plflib.org/colony.htm

template <typename T>
class Skipfield {
   private:
    T* data;
    uint32_t* skipfield;

   public:
    Skipfield(size_t size);
};
