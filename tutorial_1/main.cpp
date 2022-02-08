#include <iostream>
#include <poplar/Graph.hpp>
#include <poplar/DeviceManager.hpp>
#include <poplar/Engine.hpp>
#include <poputil/TileMapping.hpp>

using namespace std;
using namespace poplar;
using namespace poplar::program;

int main() 
{
    DeviceManager manager = DeviceManager::createDeviceManager();

    Device device;
    bool success = false;
    // Step 1 : Attach device
    for (auto &hwDevice : manager.getDevices(TargetType::IPU, 1))  // attach할 device 개수 정의
    {
        device = std::move(hwDevice);
        std::cerr << "Trying to attatch to IPU" << device.getId() << std::endl;

        if ((success = device.attach()))
        std::cerr << "Attached to IPU" << device.getId() << std::endl;
    }

    if (!success)
    {
        std::cerr << "Error attaching to device" << std::endl;
        return -1;
    }

    Target target = device.getTarget();
    Graph graph(target);

    // Add codelets to the graph
    graph.addCodelets("codelets.cpp");

    Sequence prog;
    
    Tensor result = graph.addVariable(FLOAT, {1}, "sum");
    Tensor n1 = graph.addConstant<float>(FLOAT, {1}, {1.0});
    Tensor n2 = graph.addConstant<float>(FLOAT, {1}, {2.0});
    
    //Tile mapping
    graph.setTileMapping(n1, 0);
    graph.setTileMapping(n2, 0);
    graph.setTileMapping(result, 0);

    ComputeSet tutorial_set = graph.addComputeSet("computeSet");
    auto vtx = graph.addVertex(tutorial_set, "AddNumVertex",
                                {{"x", n1[0]},
                                 {"y", n2[0]},
                                 {"sum", result[0]}});
                                 
    graph.setTileMapping(vtx, 0);

    prog.add(Execute(tutorial_set));
    prog.add(PrintTensor("sum", result));

    Engine engine(graph, prog);
    engine.load(device);
    cout << "Running program\n";
    engine.run(0);
    cout << "Program complete\n";

    return 0;
}