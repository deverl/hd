env = Environment()
env.Append(CXXFLAGS = "-Wall -pedantic --std=c++11 -O3 -DNDEBUG -DRELEASE")
env.Replace(CXX = "g++-6")
env.Program(target = "hd", source = ["hd.cpp"])
# print env.Dump()

