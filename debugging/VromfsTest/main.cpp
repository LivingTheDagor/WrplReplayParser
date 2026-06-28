



#include "VROMFs.h"

#include "DataBlock.h"
#include <chrono>
#include "reader.h"
#include <fstream>

int main() {
  fs::path war_dir = WARTHUNDER_DIR;
#ifdef Windows
  fs::path path = R"(D:\SteamLibrary\steamapps\common\develop\gameBase\aces)";
  fs::path p1 = war_dir / R"(aces.vromfs.bin)";
#endif
  /*fs::path p2 = war_dir / R"(char.vromfs.bin)";
  fs::path p3 = war_dir / R"(game.vromfs.bin)";
  fs::path p4 = war_dir / R"(gui.vromfs.bin)";
  fs::path p5 = war_dir / R"(lang.vromfs.bin)";
  fs::path p55 = war_dir / R"(mis.vromfs.bin)";
  fs::path p6 = war_dir / R"(webUi.vromfs.bin)";
  fs::path p7 = war_dir / R"(wwdata.vromfs.bin)";
  fs::path p8 = war_dir / R"(atlases.vromfs.bin)";
  fs::path p9 = war_dir / R"(fonts.vromfs.bin)";
  fs::path p10 = war_dir / R"(images.vromfs.bin)";
  fs::path p11 = war_dir / R"(slides.vromfs.bin)";
  fs::path p12 = war_dir / R"(tex.vromfs.bin)";
  fs::path p13 = war_dir / R"(regional.vromfs.bin)";*/

  EXCEPTION_IF_FALSE(file_mgr.mountVromfs(p1), "Ah shit");
  //EXCEPTION_IF_FALSE(file_mgr.loadVromfs(p2), "Ah shit");
  //EXCEPTION_IF_FALSE(file_mgr.loadVromfs(p3), "Ah shit");
  //EXCEPTION_IF_FALSE(file_mgr.loadVromfs(p4), "Ah shit");
  //EXCEPTION_IF_FALSE(file_mgr.loadVromfs(p5), "Ah shit");
  //EXCEPTION_IF_FALSE(file_mgr.loadVromfs(p55), "Ah shit");
  //EXCEPTION_IF_FALSE(file_mgr.loadVromfs(p6), "Ah shit");
  //EXCEPTION_IF_FALSE(file_mgr.loadVromfs(p7), "Ah shit");
  //EXCEPTION_IF_FALSE(file_mgr.loadVromfs(p8), "Ah shit");
  //EXCEPTION_IF_FALSE(file_mgr.loadVromfs(p9), "Ah shit");
  //EXCEPTION_IF_FALSE(file_mgr.loadVromfs(p10), "Ah shit");
  //EXCEPTION_IF_FALSE(file_mgr.loadVromfs(p11), "Ah shit");
  //EXCEPTION_IF_FALSE(file_mgr.loadVromfs(p12), "Ah shit");
  //EXCEPTION_IF_FALSE(file_mgr.loadVromfs(p13), "Ah shit");
  //EXCEPTION_IF_FALSE(file_mgr.loadVromfs(p14), "Ah shit");
  auto dir = file_mgr.getDir();

  //auto file = (*dir)["config"]["network.blk"];
  //auto v_file = std::dynamic_pointer_cast<VromfsFile>(file.asFile());
  //DataBlock blk;
  //v_file->loadBlk(blk);

  //auto file = file_mgr.getFile("templates/entities.blk");
  //std::vector<fs::path> x;
  //file_mgr.find_vromfs_files_in_folder(x, "wtlibs/templates");
  //std::exit(0);
  //auto f = (*dir)["config"]["wpcost.blk"];
  //auto file = f.asFile();
  //if (std::filesystem::exists(path)) {
  //    std::filesystem::remove_all(path);
  //}
  auto start = std::chrono::high_resolution_clock::now();
  size_t NUM_THREADS = std::thread::hardware_concurrency()-4;
  ThreadPool pool(NUM_THREADS ? NUM_THREADS : 4);

  dir->dumpToPath(path);
  pool.join();
  auto end = std::chrono::high_resolution_clock::now();

  // Calculate the duration
  std::chrono::duration<double> duration = end - start;

  std::cout << "Execution time: " << duration.count() << " seconds" << std::endl;

}
