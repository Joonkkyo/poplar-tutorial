// Copyright (c) 2018 Graphcore Ltd. All rights reserved.
#include <poplar/Vertex.hpp>

using namespace poplar;

class AddNumVertex : public Vertex
{
public:
	// Fields
    Input<float> x;
    Input<float> y;
    Output<float> sum;
    
	// Compute function
	bool compute()
	{
        *sum = x + y;
        return true;
	}
};

class SumVertex : public Vertex
{
public:
	// Fields
    Input<Vector<float>> in;
    Output<float> out;
    
	// Compute function
	bool compute()
	{
        *out = 0;
        for (const auto &v : in)
        {
            *out += v;
        }
        return true;
	}
};

class IterVertex : public Vertex
{
public:
	// Fields
    Input<Vector<float>> in;
    Output<Vector<float>> out;
    
	// Compute function
	bool compute()
	{
        for (unsigned i = 0; i < in.size(); i++)
        {
            out[i] = in[i] + 1;
        }
        return true;
	}
};




