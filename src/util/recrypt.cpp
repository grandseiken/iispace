#include <fstream>
#include <iostream>
#include <sstream>

std::string crypt(const std::string& text, const std::string& key)
{
  std::string r = "";
  for (std::size_t i = 0; i < text.length(); i++) {
    char c = text[i] ^ key[i % key.length()];
    if (text[i] == '\0' || c == '\0') {
      r += text[i];
    }
    else {
      r += c;
    }
  }
  return r;
}

std::string read_file(const std::string& filename)
{
  std::ifstream in(filename, std::ios::binary);
  std::string contents;
  in.seekg(0, std::ios::end);
  contents.resize((unsigned) in.tellg());
  in.seekg(0, std::ios::beg);
  in.read(&contents[0], contents.size());
  in.close();
  return contents;
}

int main(int argc, char** argv)
{
  if (argc < 4) {
    std::cerr <<
        "usage: " << argv[0] << " old_key_file new_key_file files..." << std::endl;
    return 1;
  }
 
  std::string old_key = read_file(argv[1]);
  std::string new_key = read_file(argv[2]);

  for (int i = 3; i < argc; ++i) {
    std::string filename = argv[i];
    std::cout << "re-encrypting " << filename << std::endl;

    std::string temp = crypt(read_file(filename), old_key);
    std::ofstream out(filename + ".recrypt", std::ios::binary);
    out << crypt(temp, new_key);
    out.close();
  }
  return 0;
}