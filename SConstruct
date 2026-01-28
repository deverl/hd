env = Environment()
env.Append(CXXFLAGS = "-Wall -pedantic --std=c++20 -O3 -DNDEBUG -DRELEASE")
env.Replace(CXX = "g++")
env.Program(target = "hd", source = ["hd.cpp"])
# print env.Dump()

