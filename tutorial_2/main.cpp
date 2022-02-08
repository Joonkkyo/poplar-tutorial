#include <iostream>
#include <poplar/Graph.hpp>
#include <poplar/DeviceManager.hpp>
#include <poplar/Engine.hpp>
#include <poputil/TileMapping.hpp>

using namespace std;
using namespace poplar;
using namespace poplar::program;

#define MAX_ITER 10

int main()
{
    DeviceManager manager = DeviceManager::createDeviceManager();

    Device device;
    bool success = false;
    // Step 1 : Attach device
    for (auto &hwDevice : manager.getDevices(TargetType::IPU, 1)) // attach할 device 개수 정의
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

    // Add variables to the graph
    Tensor v1 = graph.addVariable(FLOAT, {5}, "v1");

    for (unsigned i = 0; i < 5; ++i)
    {
        graph.setTileMapping(v1[i], i);
    }

    // Create a control program that is a sequence of steps
    Sequence prog;
    
    // Add steps to initialize the variables
    Tensor c1 = graph.addConstant<float>(FLOAT, {5}, {1.0, 2.0, 3.0, 4.0, 5.0});
    Tensor result = graph.addVariable(FLOAT, {1}, "result");

    graph.setTileMapping(c1, 0);
    graph.setTileMapping(result, 0);

    prog.add(Copy(c1, v1));

    ComputeSet tutorial_set = graph.addComputeSet("tutorial_set");
    auto vtx = graph.addVertex(tutorial_set, "SumVertex",
                                {{"in", v1},
                                 {"out", result[0]}});
    // 아래와 compute set 구성 방식이 일치한다.                       
    // VertexRef vtx = graph.addVertex(computeSet, "SumVertex");
    // graph.connect(vtx["in"], v1);
    // graph.connect(vtx["out"], result);
    graph.setTileMapping(vtx, 0);

    // Add step to execute the compute set
    prog.add(Execute(tutorial_set));
    prog.add(PrintTensor("out", result));
    // Create the engine
    Engine engine(graph, prog);
    engine.load(device);

    // Run the control program
    std::cout << "Running program\n";
    engine.run(0);
    std::cout << "Program complete\n";

    return 0;
}