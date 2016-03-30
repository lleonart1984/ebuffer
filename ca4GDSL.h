#pragma once

#define TRIANGLES D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
#define set this->setter->
#define clean Clean()->
#define with ->setter->
#define create this->builder->
#define load this->loader->
#define clear this->clearing->
#define draw this->drawer->
#define run(p) this->Run(p)
#define perform(ps, width, height) this->Perform(ps, width, height)
#define dispatch1D(x) this->Dispatch(x,1,1);
#define dispatch2D(x,y) this->Dispatch(x,y,1);
#define dispatch3D(x,y,z) this->Dispatch(x,y,z);
#define draw_indexed_triangles(vb,ib,n) this->Draw (vb, ib, n, TRIANGLES)
#define use_as(a,b) ->a=b
#define solid D3D11_FILL_SOLID
#define wireframe D3D11_FILL_WIREFRAME
#define none D3D11_CULL_NONE
#define back D3D11_CULL_BACK
#define front D3D11_CULL_FRONT

#define one D3D11_BLEND_ONE
#define zero D3D11_BLEND_ZERO
#define srcAlpha D3D11_BLEND_SRC_ALPHA
#define invSrcAlpha D3D11_BLEND_INV_SRC_ALPHA
#define dstAlpha D3D11_BLEND_DEST_ALPHA
#define invDstAlpha D3D11_BLEND_INV_DEST_ALPHA

#define addition D3D11_BLEND_OP_ADD
#define minimum D3D11_BLEND_OP_MIN
#define maximum D3D11_BLEND_OP_MAX




