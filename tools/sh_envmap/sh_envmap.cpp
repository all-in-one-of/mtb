#include <tchar.h>
#include <fstream>
#include <iostream>
#include <iomanip>

#define _USE_MATH_DEFINES
#include <cmath>

#include "hdritools\RgbeIO.h"

#include "../../src/math.hpp"
#include "../../src/common.hpp"
#include "../../src/sh.hpp"

using Rgb32F = pcg::Rgb32F;

const float PI = 3.14159265358979323846f;

struct uv01 {
	float u;
	float v;
};

struct uv_11 {
	float u;
	float v;
};

struct polar {
	float theta;
	float phi;
};

// Coordinate system conversion formulae from "High Dynamic Range Imaging" by Reinhard et al., chapter 9.4
polar get_lat_lon_polar(float u02, float v01) {
	return { PI * (u02 - 1), PI * v01 };
}

vec3 get_lat_lon_world(polar p) {
	float sinPhi = ::sinf(p.phi);
	float cosPhi = ::cosf(p.phi);
	float sinTheta = ::sinf(p.theta);
	float cosTheta = ::cosf(p.theta);

	return{ sinPhi * sinTheta, cosPhi, -sinPhi * cosTheta };
}

uv01 get_angular_uv01(vec3 w) {
	float r = ::acosf(-w.z) / (2 * PI * ::sqrtf(w.x*w.x + w.y*w.y));
	float u = 0.5f + r * w.x;
	float v = 0.5f - r * w.y;
	return{ u, v };
}

polar get_angular_polar(uv_11 uv) {
	float theta = ::atan2f(-uv.v, uv.u);
	float phi = PI * ::sqrtf(uv.u*uv.u + uv.v*uv.v);
	return {theta, phi};
}

vec3 get_angular_world(polar p) {
	float sinPhi = ::sinf(p.phi);
	float cosPhi = ::cosf(p.phi);
	float sinTheta = ::sinf(p.theta);
	float cosTheta = ::cosf(p.theta);

	float x = sinPhi * cosTheta;
	float y = sinPhi * sinTheta;
	float z = -cosPhi;

	return { x, y, z };
}

// Based on "An Efficient Representation for Irradiance Environment Maps"
// by Ramamoorthi and Hanrahan and their code example prefilter.c
sSHCoef calc_sh_coef(pcg::Image<pcg::Rgb32F, pcg::TopDown> const& angular) {
	sSHCoef sh;
	sh.zero();

	int w = angular.Width();
	int h = angular.Height();

	float wHalf = w * 0.5f;
	float hHalf = h * 0.5f;

	float W = (float)w;
	float H = (float)h;

	auto sinc = [](float x) {
		if (::fabs(x) < 1e-4) { return 1.0f; }
		return ::sinf(x) / x;
	};

	for (int row = 0; row < h; ++row) {
		for (int col = 0; col < w; ++col) {
			float u = (col - wHalf) / wHalf;
			float v = (row - hHalf) / hHalf;

			if (u*u + v*v > 1.0f) { continue; }

			polar p = get_angular_polar(uv_11{ u, v });
			vec3 world = get_angular_world(p);

			//Rgb32F c = { (world.x + 1) * 0.5f, (world.y + 1) * 0.5f, (world.z + 1) * 0.5f };
			//Rgb32F c = { world.x + 10, world.y + 10, world.z + 10 };
			//angular.ElementAt(col, row) = c;

			world.z = -world.z;

			Rgb32F c = angular.ElementAt(col, row);

			float dOmega = (2 * PI / W) * (2 * PI / H) * sinc(p.phi);
			
			sh.addProbeSample(world, c.r, c.g, c.b, dOmega);
		}
	}

	return sh;
}

void dump_sh_as_geo(sSHCoef const& sh, ostream& os) {
	os << "PGEOMETRY V5\n";
	os << "NPoints 0 NPrims 0\n";
	os << "NPointGroups 0 NPrimGroups 0\n";
	os << "NPointAttrib 0 NVertexAttrib 0 NPrimAttrib 0 NAttrib 1\n";
	os << "DetailAttrib\n";
	os << "shcoef 27 float 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n";
	//os << "varmap 1 index 1 \"shcoef -> SHCOEF\"\n";

	os << " (";
	os << std::setprecision(20);
	for (int i = 0; i < 3; ++i) {
		auto const& chan = sh.get_chan(i);
		for (int j = 0; j < 9; ++j) {
			os << chan[j] << " ";
		}
	}
	os << ")\n";
	os << "beginExtra\n";
	os << "endExtra\n";
}

void dump_sh_as_code(sSHCoef const& sh, ostream& os) {
	char const* const names[3] = { "mR", "mG", "mB" };
	os << "sSHCoef sh;\n";
	os << std::setprecision(20);
	for (int i = 0; i < 3; ++i) {
		auto const& chan = sh.get_chan(i);
		os << "sh." << names[i] << " = {{ ";
		for (int j = 0; j < 9; ++j) {
			if (j > 0)
				os << ", ";
			os << chan[j] << "f";
		}
		os << " }};\n";
	}
}

void angular_to_lat_lon(pcg::Image<pcg::Rgb32F, pcg::TopDown> const& angular, pcg::Image<pcg::Rgb32F, pcg::TopDown>& latLon) {
	int aw = angular.Width();
	int ah = angular.Height();

	int w = aw * 2;
	int h = ah;

	latLon.Alloc(w, h);

	float wHalf = w * 0.5f;
	float hHalf = h * 0.5f;
	float W = (float)w;
	float H = (float)h;

	for (int row = 0; row < h; ++row) {
		for (int col = 0; col < w; ++col) {
			float u = col / wHalf;
			float v = row / H;

			polar pll = get_lat_lon_polar(u, v);
			vec3 world = get_lat_lon_world(pll);
			uv01 auv = get_angular_uv01(world);

			int arow = (int)(auv.v * (ah - 1));
			int acol = (int)(auv.u * (aw - 1));

			latLon.ElementAt(col, row) = angular.ElementAt(acol, arow);
		}
	}
}



int main(int argc, char* argv[]) try {
	if (argc < 2) {
		std::cerr << "Not enough argumetns" << std::endl;
		return 1;
	}

	auto const* filepath = argv[1];

	std::ifstream ifs;
	ifs.open(filepath, std::ios_base::binary);
	if (!ifs.is_open()) {
		std::cerr << "Unable to open file" << std::endl;
		return 2;
	}

	pcg::Image<pcg::Rgb32F, pcg::TopDown> img;
	pcg::RgbeIO::Load(img, ifs);
	ifs.close();

	sSHCoef sh = calc_sh_coef(img);
	//pcg::RgbeIO::Save(img, "test.hdr");

	//auto const* data = img.GetDataPointer();

	//pcg::Image<pcg::Rgb32F, pcg::TopDown> latLon;
	//angular_to_lat_lon(img, latLon);
	//pcg::RgbeIO::Save(latLon, "latlon.hdr");

	char nameBuf[128];
	::_splitpath_s(filepath, nullptr, 0, nullptr, 0, nameBuf, sizeof(nameBuf), nullptr, 0);
	char buf[128];

	::sprintf_s(buf, "%s.geo", nameBuf);
	std::cout << "Writing " << buf << std::endl;
	std::ofstream geoOfs;
	geoOfs.open(buf);
	if (!geoOfs.is_open()) {
		std::cerr << "Unable to open file " << buf << std::endl;
		return 3;
	}
	dump_sh_as_geo(sh, geoOfs);
	geoOfs.close();

	std::ofstream codeOfs;
	::sprintf_s(buf, "%s.code", nameBuf);
	std::cout << "Writing " << buf << std::endl;
	codeOfs.open(buf);
	if (!codeOfs.is_open()) {
		std::cerr << "Unable to open file " << buf << std::endl;
		return 3;
	}
	dump_sh_as_code(sh, codeOfs);
	codeOfs.close();

	return 0;
}
catch (std::exception& e) {
	std::cerr << e.what() << std::endl;
	return 3;
}

