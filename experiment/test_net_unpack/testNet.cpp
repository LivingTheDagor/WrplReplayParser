#include "daNet/dag_netUtils.h"
#include "math/dag_Point3.h"
#include "math/dag_Quat.h"

template <typename T, uint8_t shift>
T shld(T shifted, T bit_filler)
{
  constexpr uint32_t bit_size = sizeof(T)*8;
  T out = shifted << shift;
  out |= bit_filler >> (bit_size-shift);
  return out;
}

void ParseLocation(Point3 &point) {
  constexpr float xmm13 = 1.0f; // asummed to always be 1.0f
  constexpr float xmm12 = -99.9;
  constexpr float FLOAT_146020698 = 4.768372718899627e-07;
  constexpr float FLOAT_146020650 = 36000.0;
  constexpr float FLOAT_14602069c = 0.007629402;
  uint32_t eax = 0x000000008B5D240E;
  uint32_t ecx = 0x000000000E62693A;
  uint32_t edx = eax; // move edx, eax
  edx = shld<uint32_t, 0xa>(edx, ecx); // shld edx, ecx, A
  edx &= 0xFFFFF; // and edx, FFFFF
  eax >>= 0xa; // shr eax, A
  float xmm0 = 0; // xorps xmm0, xmm0
  xmm0 = (float)eax; // cvtsi2ss xmm0, eax
  float xmm3 = FLOAT_146020698; // movss xmm3,dword ptr ds:[7FF7266E0698]
  xmm0 = xmm0 * xmm3; //mulss xmm0,xmm3
  float xmm2 = -1.0f; // movss xmm2,dword ptr ds:[7FF7265FF220]
  xmm0 = xmm0 +  xmm2; // addss xmm0, xmm2
  float xmm1 = xmm2; // movaps xmm1, xmm2
  xmm1 = std::max(xmm1, xmm0); // maxss xmm1, xmm0
  xmm1 = std::min(xmm1, xmm13); // minss xmm1, xmm13
  xmm0 = 0; // xoprs, xmm0, xmm0
  xmm0 = (float)edx; // cvtsi2ss xmm0, eax
  float xmm4 = FLOAT_146020650; // movss xmm4,dword ptr ds:[7FF7266E0650]
  xmm1 *= xmm4; // mulss xmm1,xmm4
  point.x = xmm1; // movss dword ptr ss:[rsp+C8],xmm1
  xmm0 *=FLOAT_14602069c; // mulss xmm0,dword ptr ds:[7FF7266E069C]
  xmm0 += xmm12; // addss xmm0,xmm12
  ecx &= 0x3FFFFF; // and ecx,3FFFFF
  xmm1 = 0; // xorps xmm1,xmm1
  xmm1 = (float)ecx; // cvtsi2ss xmm1,ecx
  point.y = xmm0; // movss dword ptr ss:[rsp+CC],xmm0
  xmm1 *= xmm3; // mulss xmm1,xmm3
  xmm1 += xmm2; // addss xmm1,xmm2
  xmm0 = xmm2; // movaps xmm0,xmm2
  xmm0 = std::max(xmm0, xmm1);
  xmm0 = std::min(xmm0, xmm13);
  xmm0 *= xmm4;
  point.z = xmm0;
  LOG("{}; {}; {}\n", point.x, point.y, point.z);
}

int main() {
  //std::array<int16_t, 3> bytes{0x6106, static_cast<short>(0xa500), static_cast<short>(0xf7ff)};
  //std::array<int16_t, 3> bytes_2{static_cast<short>(0xe08e), static_cast<short>(0xdc84), 0x70cf};
  /*Point3 out{};
  Quat out2{};
  netutils::unpack_euler_16(bytes, out);
  LOG("%f; %f; %f\n", out.x, out.y, out.z);
  uint32_t v = 0xb91f0ac3;
  netutils::unpack_euler(v, out);
  LOG("%f; %f; %f\n", out.x, out.y, out.z);
  uint32_t v2 = 0x10e2aebe;
  netutils::unpack_euler(v2, out);
  LOG("%f; %f; %f\n", out.x, out.y, out.z);
  uint32_t v3 = 0xb91f0ac3;
  netutils::unpack_quat(v3, out2);
  LOG("%f; %f; %f; %f\n", out2.x, out2.y, out2.z, out2.w);*/

  //netutils::unpack_euler_16(bytes_2, out);

  //  std::cout.flush();

  return 0;
}
