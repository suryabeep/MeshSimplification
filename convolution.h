
class HeightmapGenerator {
public:
  HeightmapGenerator(float* noiseValues);
  
  void run();
  
  float* get(void) {
    return output_h;
  };

  float getMaxHeight();
  float getMinHeight();

  ~HeightmapGenerator();

private:
  float* mask_h;
  float* output_h;
  float* input_d;
  float* output_d;
  float  minHeight;
  float  maxHeight;
};
