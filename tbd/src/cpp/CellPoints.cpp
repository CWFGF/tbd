/* Copyright (c) Jordan Evens, 2005, 2021 */
/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2021-2024. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "CellPoints.h"
#include "Log.h"
namespace tbd::sim
{
static const double MIN_X = std::numeric_limits<double>::min();
static const double MAX_X = std::numeric_limits<double>::max();
// const double TAN_PI_8 = std::tan(std::numbers::pi / 8);
// const double LEN_PI_8 = TAN_PI_8 / 2;
constexpr double DIST_22_5 = 0.2071067811865475244008443621048490392848359376884740365883398689;
constexpr double P_0_5 = 0.5 + DIST_22_5;
constexpr double M_0_5 = 0.5 - DIST_22_5;

inline constexpr double distPtPt(const tbd::sim::InnerPos& a, const tbd::sim::InnerPos& b) noexcept
{
#ifdef _WIN32
  return (((b.x() - a.x()) * (b.x() - a.x())) + ((b.y() - a.y()) * (b.y() - a.y())));
#else
  return (std::pow((b.x() - a.x()), 2) + std::pow((b.y() - a.y()), 2));
#endif
}

CellPoints::CellPoints() noexcept
  : pts_(),
    dists_()
{
  std::fill_n(dists_.begin(), NUM_DIRECTIONS, numeric_limits<double>::max());
}

CellPoints::CellPoints(const double x, const double y) noexcept
  : CellPoints()
{
  insert(InnerPos{x, y});
}

CellPoints::CellPoints(const InnerPos& p) noexcept
  : CellPoints()
{
  insert(p);
}

void CellPoints::insert(const InnerPos& p) noexcept
{
  // should always be in the same cell so do this once
  const auto cell_x = static_cast<tbd::Idx>(p.x());
  const auto cell_y = static_cast<tbd::Idx>(p.y());
  insert(cell_x, cell_y, p);
}
void CellPoints::insert(const double cell_x, const double cell_y, const InnerPos& p) noexcept
{
  auto& n = dists_[FURTHEST_N];
  auto& nne = dists_[FURTHEST_NNE];
  auto& ne = dists_[FURTHEST_NE];
  auto& ene = dists_[FURTHEST_ENE];
  auto& e = dists_[FURTHEST_E];
  auto& ese = dists_[FURTHEST_ESE];
  auto& se = dists_[FURTHEST_SE];
  auto& sse = dists_[FURTHEST_SSE];
  auto& s = dists_[FURTHEST_S];
  auto& ssw = dists_[FURTHEST_SSW];
  auto& sw = dists_[FURTHEST_SW];
  auto& wsw = dists_[FURTHEST_WSW];
  auto& w = dists_[FURTHEST_W];
  auto& wnw = dists_[FURTHEST_WNW];
  auto& nw = dists_[FURTHEST_NW];
  auto& nnw = dists_[FURTHEST_NNW];
  auto& n_pos = pts_[FURTHEST_N];
  auto& nne_pos = pts_[FURTHEST_NNE];
  auto& ne_pos = pts_[FURTHEST_NE];
  auto& ene_pos = pts_[FURTHEST_ENE];
  auto& e_pos = pts_[FURTHEST_E];
  auto& ese_pos = pts_[FURTHEST_ESE];
  auto& se_pos = pts_[FURTHEST_SE];
  auto& sse_pos = pts_[FURTHEST_SSE];
  auto& s_pos = pts_[FURTHEST_S];
  auto& ssw_pos = pts_[FURTHEST_SSW];
  auto& sw_pos = pts_[FURTHEST_SW];
  auto& wsw_pos = pts_[FURTHEST_WSW];
  auto& w_pos = pts_[FURTHEST_W];
  auto& wnw_pos = pts_[FURTHEST_WNW];
  auto& nw_pos = pts_[FURTHEST_NW];
  auto& nnw_pos = pts_[FURTHEST_NNW];
  const auto x = p.x() - cell_x;
  const auto y = p.y() - cell_y;
  // north is closest to point (0.5, 1.0)
  const auto cur_n = ((x - 0.5) * (x - 0.5)) + ((1 - y) * (1 - y));
  if (cur_n < n)
  {
    n_pos = p;
    n = cur_n;
  }
  // south is closest to point (0.5, 0.0)
  const auto cur_s = ((x - 0.5) * (x - 0.5)) + (y * y);
  if (cur_s < s)
  {
    s_pos = p;
    s = cur_s;
  }
  // northeast is closest to point (1.0, 1.0)
  const auto cur_ne = ((1 - x) * (1 - x)) + ((1 - y) * (1 - y));
  if (cur_ne < ne)
  {
    ne_pos = p;
    ne = cur_ne;
  }
  // southwest is closest to point (0.0, 0.0)
  const auto cur_sw = (x * x) + (y * y);
  if (cur_sw < sw)
  {
    sw_pos = p;
    sw = cur_sw;
  }
  // east is closest to point (1.0, 0.5)
  const auto cur_e = ((1 - x) * (1 - x)) + ((y - 0.5) * (y - 0.5));
  if (cur_e < e)
  {
    e_pos = p;
    e = cur_e;
  }
  // west is closest to point (0.0, 0.5)
  const auto cur_w = (x * x) + ((y - 0.5) * (y - 0.5));
  if (cur_w < w)
  {
    w_pos = p;
    w = cur_w;
  }
  // southeast is closest to point (1.0, 0.0)
  const auto cur_se = ((1 - x) * (1 - x)) + (y * y);
  if (cur_se < se)
  {
    se_pos = p;
    se = cur_se;
  }
  // northwest is closest to point (0.0, 1.0)
  const auto cur_nw = (x * x) + ((1 - y) * (1 - y));
  if (cur_nw < nw)
  {
    nw_pos = p;
    nw = cur_nw;
  }
  // south-southwest is closest to point (0.5 - 0.207, 0.0)
  const auto cur_ssw = ((x - M_0_5) * (x - M_0_5)) + (y * y);
  if (cur_ssw < ssw)
  {
    ssw_pos = p;
    ssw = cur_ssw;
  }
  // south-southeast is closest to point (0.5 + 0.207, 0.0)
  const auto cur_sse = ((x - P_0_5) * (x - P_0_5)) + (y * y);
  if (cur_sse < sse)
  {
    sse_pos = p;
    sse = cur_sse;
  }
  // north-northwest is closest to point (0.5 - 0.207, 1.0)
  const auto cur_nnw = ((x - M_0_5) * (x - M_0_5)) + ((1 - y) * (1 - y));
  if (cur_nnw < nnw)
  {
    nnw_pos = p;
    nnw = cur_nnw;
  }
  // north-northeast is closest to point (0.5 + 0.207, 1.0)
  const auto cur_nne = ((x - P_0_5) * (x - P_0_5)) + ((1 - y) * (1 - y));
  if (cur_nne < nne)
  {
    nne_pos = p;
    nne = cur_nne;
  }
  // west-southwest is closest to point (0.0, 0.5 - 0.207)
  const auto cur_wsw = (x * x) + ((y - M_0_5) * (y - M_0_5));
  if (cur_wsw < wsw)
  {
    wsw_pos = p;
    wsw = cur_wsw;
  }
  // west-northwest is closest to point (0.0, 0.5 + 0.207)
  const auto cur_wnw = (x * x) + ((y - P_0_5) * (y - P_0_5));
  if (cur_wnw < wnw)
  {
    wnw_pos = p;
    wnw = cur_wnw;
  }
  // east-southeast is closest to point (1.0, 0.5 - 0.207)
  const auto cur_ese = ((1 - x) * (1 - x)) + ((y - M_0_5) * (y - M_0_5));
  if (cur_ese < ese)
  {
    ese_pos = p;
    ese = cur_ese;
  }
  // east-northeast is closest to point (1.0, 0.5 + 0.207)
  const auto cur_ene = ((1 - x) * (1 - x)) + ((y - P_0_5) * (y - P_0_5));
  if (cur_ene < ene)
  {
    ene_pos = p;
    ene = cur_ene;
  }
}
CellPoints::CellPoints(const vector<InnerPos>& pts) noexcept
  : CellPoints()
{
  // should always be in the same cell so do this once
  const auto cell_x = static_cast<tbd::Idx>(pts[0].x());
  const auto cell_y = static_cast<tbd::Idx>(pts[0].y());
  for (auto& p : pts)
  {
    insert(cell_x, cell_y, p);
  }
}
}