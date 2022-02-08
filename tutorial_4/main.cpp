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
    Tensor v1 = graph.addVariable(FLOAT, {4}, "v1");
    Tensor v2 = graph.addVariable(FLOAT, {4}, "v2");
    for (unsigned i = 0; i < 4; ++i)
    {
        graph.setTileMapping(v1[i], i);
        graph.setTileMapping(v2[i], i);
    }

    // Create a control program that is a sequence of steps
    Sequence prog;

    // Add steps to initialize the variables
    Tensor c1 = graph.addConstant<float>(FLOAT, {4}, {1.0, 1.5, 2.0, 2.5});
    graph.setTileMapping(c1, 0);
    prog.add(Copy(c1, v1));

    ComputeSet computeSet = graph.addComputeSet("computeSet");
    for (unsigned i = 0; i < 4; ++i)
    {
        VertexRef vtx = graph.addVertex(computeSet, "SumVertex");
        cout << v1.slice(i, 4) << endl;
        graph.connect(vtx["in"], v1.slice(i, 4));
        graph.connect(vtx["out"], v2[i]);
        graph.setTileMapping(vtx, i);
    }

    // Add step to execute the compute set
    prog.add(Execute(computeSet));

    // Add step to print out v2
    prog.add(PrintTensor("v2", v2));

    // Create the engine
    Engine engine(graph, prog);
    engine.load(device);

    // Run the control program
    std::cout << "Running program\n";
    engine.run(0);
    std::cout << "Program complete\n";

    return 0;
}