/* Copyright (c) Jordan Evens, 2005, 2021 */
/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2021-2024. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "CellPoints.h"
#include "Log.h"
#include "ConvexHull.h"
#include "Location.h"

namespace tbd::sim
{
static const double MIN_X = std::numeric_limits<double>::min();
static const double MAX_X = std::numeric_limits<double>::max();
// const double TAN_PI_8 = std::tan(std::numbers::pi / 8);
// const double LEN_PI_8 = TAN_PI_8 / 2;
constexpr double DIST_22_5 = 0.2071067811865475244008443621048490392848359376884740365883398689;
constexpr double P_0_5 = 0.5 + DIST_22_5;
constexpr double M_0_5 = 0.5 - DIST_22_5;
//   static constexpr double INVALID_DISTANCE = std::numeric_limits<double>::max();
// not sure what's going on with this and wondering if it doesn't keep number exactly
// shouldn't be any way to be further than twice the entire width of the area
static const double INVALID_DISTANCE = static_cast<double>(MAX_ROWS * MAX_ROWS);
static const pair<double, InnerPos> INVALID_PAIR{INVALID_DISTANCE, {}};
static const Idx INVALID_LOCATION = INVALID_PAIR.second.x();
void assert_all_equal(
  const CellPoints::array_dists& pts,
  const double x,
  const double y)
{
  for (size_t i = 0; i < pts.size(); ++i)
  {
    logging::check_equal(pts[i].second.x(), x, "point x");
    logging::check_equal(pts[i].second.y(), y, "point y");
  }
}
void assert_all_invalid(const CellPoints::array_dists& pts)
{
  for (size_t i = 0; i < pts.size(); ++i)
  {
    logging::check_equal(INVALID_DISTANCE, pts[i].first, "distances");
  }
  assert_all_equal(pts, INVALID_LOCATION, INVALID_LOCATION);
}
set<InnerPos> CellPoints::unique() const noexcept
{
  set<InnerPos> result{};
  for (size_t i = 0; i < pts_.size(); ++i)
  {
    if (INVALID_DISTANCE != pts_[i].first)
    {
      const auto& p = pts_[i].second;
#ifdef DEBUG_POINTS
      const Location loc{cell_y_, cell_x_};
      const Location loc1{static_cast<Idx>(p.y()), static_cast<Idx>(p.x())};
      logging::check_equal(
        loc1.column(),
        loc.column(),
        "column");
      logging::check_equal(
        loc1.row(),
        loc.row(),
        "row");
#endif
      result.emplace(p);
    }
  }
#ifdef DEBUG_POINTS
  if (result.empty())
  {
    assert_all_invalid(pts_);
  }
#endif
  return result;
}
CellPoints::CellPoints(const Idx cell_x, const Idx cell_y) noexcept
  : pts_({}),
    cell_x_(cell_x),
    cell_y_(cell_y),
    src_(topo::DIRECTION_NONE)
{
  std::fill(pts_.begin(), pts_.end(), INVALID_PAIR);
#ifdef DEBUG_POINTS
  assert_all_invalid(pts_);
#endif
}
CellPoints::CellPoints() noexcept
  : CellPoints(INVALID_LOCATION, INVALID_LOCATION)
{
  // already done but check again since debugging
  assert_all_invalid(pts_);
}

// CellPoints::CellPoints(size_t) noexcept
//   : CellPoints()
// {
// }
CellPoints::CellPoints(const CellPoints* rhs) noexcept
  : CellPoints()
{
  if (nullptr != rhs)
  {
#ifdef DEBUG_POINTS
    bool rhs_empty = rhs->unique().empty();
#endif
    merge(*rhs);
#ifdef DEBUG_POINTS
    logging::check_equal(
      unique().empty(),
      rhs_empty,
      "empty");
#endif
  }
#ifdef DEBUG_POINTS
  else
  {
    assert_all_invalid(pts_);
  }
#endif
}
CellPoints::CellPoints(const double x, const double y) noexcept
  : CellPoints(static_cast<Idx>(x), static_cast<Idx>(y))
{
#ifdef DEBUG_POINTS
  assert_all_invalid(pts_);
#endif
  insert_(x, y);
#ifdef DEBUG_POINTS
  assert_all_equal(pts_, x, y);
#endif
}

CellPoints& CellPoints::insert(const double x, const double y) noexcept
{
#ifdef DEBUG_POINTS
  const bool was_empty = empty();
  logging::check_fatal(
    is_invalid(),
    "Invalid when calling insert(%f, %f)",
    x,
    y);
#endif
  insert_(x, y);
#ifdef DEBUG_POINTS
  logging::check_fatal(unique().empty(), "Empty after insert of (%f, %f)", x, y);
  //   set<InnerPos> result{};
  if (was_empty)
  {
    assert_all_equal(pts_, x, y);
  }
  else
  {
    for (size_t i = 0; i < pts_.size(); ++i)
    {
      logging::check_fatal(
        INVALID_DISTANCE == pts_[i].first,
        "Invalid distance at position %ld",
        i);
    }
  }
#endif
  return *this;
}
CellPoints::CellPoints(const InnerPos& p) noexcept
  : CellPoints(p.x(), p.y())
{
  assert_all_equal(pts_, p.x(), p.y());
}

CellPoints& CellPoints::insert(const InnerPos& p) noexcept
{
#ifdef DEBUG_POINTS
  const auto x = p.x();
  const auto y = p.y();
  const bool was_empty = empty();
  logging::check_fatal(
    is_invalid(),
    "Invalid when calling insert(%f, %f)",
    x,
    y);
#endif
  insert(p.x(), p.y());
#ifdef DEBUG_POINTS
  logging::check_fatal(unique().empty(), "Empty after insert of (%f, %f)", x, y);
  //   set<InnerPos> result{};
  if (was_empty)
  {
    assert_all_equal(pts_, x, y);
  }
  else
  {
    for (size_t i = 0; i < pts_.size(); ++i)
    {
      logging::check_fatal(
        INVALID_DISTANCE == pts_[i].first,
        "Invalid distance at position %ld",
        i);
    }
  }
#endif
  return *this;
}
CellPoints::array_dists CellPoints::find_distances(const double p_x, const double p_y) noexcept
{
#ifdef DEBUG_GRIDS
  logging::check_fatal(
    p_y < 0 || p_y >= MAX_ROWS,
    "p_y %f is out of bounds (%f, %f)",
    p_y,
    0,
    MAX_ROWS);
  logging::check_fatal(
    p_x < 0 || p_x >= MAX_COLUMNS,
    "p_x %f is out of bounds (%f, %f)",
    p_x,
    0,
    MAX_COLUMNS);
#endif
  //   const InnerPos p{p_x, p_y};
  const auto x = p_x - cell_x_;
  const auto y = p_y - cell_y_;
#ifdef DEBUG_GRIDS
  logging::check_fatal(
    x < 0 || x > 1,
    "x %f is out of cell (%f, %f)",
    x,
    0,
    1);
  logging::check_fatal(
    y < 0 || y > 1,
    "y %f is out of cell (%f, %f)",
    y,
    0,
    1);
#endif
#define DISTANCE_1D(a, b) (((a) - (b)) * ((a) - (b)))
#define DISTANCE_XY(x0, y0) (DISTANCE_1D((x), (x0)) + DISTANCE_1D((y), (y0)))
#define DISTANCE(x0, y0) (pair<double, InnerPos>{DISTANCE_XY(x0, y0), InnerPos{p_x, p_y}})
#ifdef DEBUG_POINTS
  const auto dist_self = DISTANCE(x, y);
  logging::check_equal(
    0,
    dist_self.first,
    "distance to self");
  logging::check_equal(
    p_x,
    dist_self.second.x(),
    "x from distance to self");
  logging::check_equal(
    p_y,
    dist_self.second.y(),
    "y from distance to self");
#endif
  // NOTE: order of x0/x and y0/y shouldn't matter since squaring
  return {
    // north is closest to point (0.5, 1.0)
    DISTANCE(0.5, 1.0),
    // north-northeast is closest to point (0.5 + 0.207, 1.0)
    DISTANCE(P_0_5, 1.0),
    // northeast is closest to point (1.0, 1.0)
    DISTANCE(1.0, 1.0),
    // east-northeast is closest to point (1.0, 0.5 + 0.207)
    DISTANCE(1.0, P_0_5),
    // east is closest to point (1.0, 0.5)
    DISTANCE(1.0, 0.5),
    // east-southeast is closest to point (1.0, 0.5 - 0.207)
    DISTANCE(1.0, M_0_5),
    // southeast is closest to point (1.0, 0.0)
    DISTANCE(1.0, 0),
    // south-southeast is closest to point (0.5 + 0.207, 0.0)
    DISTANCE(P_0_5, 0.0),
    // south is closest to point (0.5, 0.0)
    DISTANCE(0.5, 0.0),
    // south-southwest is closest to point (0.5 - 0.207, 0.0)
    DISTANCE(M_0_5, 0.0),
    // southwest is closest to point (0.0, 0.0)
    DISTANCE(0.0, 0.0),
    // west-southwest is closest to point (0.0, 0.5 - 0.207)
    DISTANCE(0.0, M_0_5),
    // west is closest to point (0.0, 0.5)
    DISTANCE(0.0, 0.5),
    // west-northwest is closest to point (0.0, 0.5 + 0.207)
    DISTANCE(0.0, P_0_5),
    // northwest is closest to point (0.0, 1.0)
    DISTANCE(0.0, 1.0),
    // north-northwest is closest to point (0.5 - 0.207, 1.0)
    DISTANCE(M_0_5, 1.0)};
#undef DISTANCE_1D
#undef DISTANCE
}
CellPoints& CellPoints::insert_(const double x, const double y) noexcept
{
#ifdef DEBUG_POINTS
  logging::check_fatal(
    is_invalid(),
    "Expected cell_x_ and cell_y_ to be set before calling insert_ (%f, %f)",
    x,
    y);
  logging::check_equal(static_cast<Idx>(x), cell_x_, "CellPoints x");
  logging::check_equal(static_cast<Idx>(y), cell_y_, "CellPoints y");
  logging::check_fatal(
    x < 0 || x >= MAX_COLUMNS,
    "x %f is out of bounds (%f, %f)",
    x,
    0,
    MAX_COLUMNS);
  logging::check_fatal(
    y < 0 || y >= MAX_ROWS,
    "y %f is out of bounds (%f, %f)",
    y,
    0,
    MAX_ROWS);
#endif
  array_dists dists = find_distances(x, y);
  for (size_t i = 0; i < dists.size(); ++i)
  {
#ifdef DEBUG_POINTS
    logging::check_fatal(
      INVALID_DISTANCE == dists[i].first,
      "Invalid distance returned from find_distances(%f, %f) for %ld",
      x,
      y,
      i);
    logging::check_equal(
      x,
      dists[i].second.x(),
      "distance pair x");
    logging::check_equal(
      y,
      dists[i].second.y(),
      "distance pair y");
#endif
    // NOTE: comparing pair will look at distance first
    pts_[i] = min(pts_[i], dists[i]);
#ifdef DEBUG_POINTS
    if (pts_[i].first == dists[i].first)
    {
      logging::check_equal(
        pts_[i].second.x(),
        dists[i].second.x(),
        "distance pair x");
      logging::check_equal(
        pts_[i].second.y(),
        dists[i].second.y(),
        "distance pair y");
    }
#endif
  }
  return *this;
}
// CellPoints::CellPoints(const vector<InnerPos>& pts) noexcept
//   : CellPoints(
//       static_cast<Idx>((*pts.begin()).x()),
//       static_cast<Idx>((*pts.begin()).y()))
// {
// #ifdef DEBUG_POINTS
//   const auto& pt = *pts.begin();
//   // make sure cell_x_ and cell_y_ are set properly from first item
//   logging::check_equal(
//     static_cast<Idx>(pt.x()),
//     cell_x_,
//     "cell_x_");
//   logging::check_equal(
//     static_cast<Idx>(pt.y()),
//     cell_y_,
//     "cell_y_");
// #endif
//   insert(pts.begin(), pts.end());
// }
void CellPoints::add_source(const CellIndex src)
{
#ifdef DEBUG_POINTS
#endif
  src_ |= src;
#ifdef DEBUG_POINTS
  // logical and produces input
  logging::check_equal(
    src_ & src,
    src,
    "source mask");
  logging::check_equal(
    !(src_ & src),
    !src,
    "source non-zero");
#endif
}
CellPoints& CellPoints::merge(const CellPoints& rhs)
{
#ifdef DEBUG_POINTS
  logging::check_fatal(
    !((rhs.is_invalid()
       || is_invalid())
      || (cell_x_ == rhs.cell_x_
          && cell_y_ == rhs.cell_y_)),
    "Expected for_cell_ to be the same or at least one invalid but have (%d, %d) and (%d, %d)",
    cell_x_,
    cell_y_,
    rhs.cell_x_,
    rhs.cell_y_);
  const auto prev_x = cell_x_;
  const auto prev_y = cell_y_;
#endif
  // either both invalid or lower one is valid
  cell_x_ = min(cell_x_, rhs.cell_x_);
  cell_y_ = min(cell_y_, rhs.cell_y_);
#ifdef DEBUG_POINTS
  if (INVALID_LOCATION == rhs.cell_x_)
  {
    logging::check_equal(
      cell_x_,
      prev_x,
      "orig cell_x_");
    logging::check_equal(
      cell_y_,
      prev_y,
      "orig cell_y_");
  }
  if (INVALID_LOCATION == rhs.cell_y_)
  {
    logging::check_equal(
      cell_x_,
      prev_x,
      "orig cell_x_");
    logging::check_equal(
      cell_y_,
      prev_y,
      "orig cell_y_");
  }
  if (INVALID_LOCATION == prev_x)
  {
    logging::check_equal(
      cell_x_,
      rhs.cell_x_,
      "orig cell_x_");
    logging::check_equal(
      cell_y_,
      rhs.cell_y_,
      "orig cell_y_");
  }
  if (INVALID_LOCATION == prev_y)
  {
    logging::check_equal(
      cell_x_,
      rhs.cell_x_,
      "orig cell_x_");
    logging::check_equal(
      cell_y_,
      rhs.cell_y_,
      "orig cell_y_");
  }
  if (cell_x_ == rhs.cell_x_)
  {
    logging::check_equal(
      cell_y_,
      rhs.cell_y_,
      "merged rhs cell_y_");
  }
  if (cell_x_ == prev_x)
  {
    logging::check_equal(
      cell_y_,
      prev_y,
      "merged rhs cell_y_");
  }
#endif
  // we know distances in each direction so just pick closer
  for (size_t i = 0; i < pts_.size(); ++i)
  {
    pts_[i] = min(pts_[i], rhs.pts_[i]);
  }
  add_source(rhs.src_);
  return *this;
}
CellPointsMap apply_offsets_spreadkey(
  const double duration,
  const OffsetSet& offsets,
  const spreading_points::mapped_type& cell_pts)
{
  // NOTE: really tried to do this in parallel, but not enough points
  // in a cell for it to work well
  CellPointsMap r1{};
#ifdef DEBUG_POINTS
  logging::check_fatal(
    offsets.empty(),
    "offsets.empty()");
  const auto s0 = offsets.size();
  const auto s1 = cell_pts.size();
  logging::check_fatal(
    0 == s0,
    "Applying no offsets");
  logging::check_fatal(
    0 == s1,
    "Applying offsets to no points");
#endif
  // apply offsets to point
  for (const auto& out : offsets)
  {
    const double x_o = duration * out.x();
    const double y_o = duration * out.y();
#ifdef DEBUG_POINTS
    logging::check_fatal(
      cell_pts.empty(),
      "cell_pts.empty()");
#endif
    for (const auto& pts_for_cell : cell_pts)
    {
      const Location& src = std::get<0>(pts_for_cell);
      const CellPoints& pts = std::get<1>(pts_for_cell);
      const auto& u = pts.unique();
#ifdef DEBUG_POINTS
      const Location loc1{src.row(), src.column()};
      logging::check_equal(
        src.hash(),
        loc1.hash(),
        "hash");
      logging::check_fatal(
        u.size() > NUM_DIRECTIONS,
        "Expected less than %d unique points but have %ld",
        NUM_DIRECTIONS,
        u.size());
      logging::check_fatal(
        u.empty(),
        "Should not have empty CellPoints");
#endif
      for (const auto& p : u)
      {
        // putting results in copy of offsets and returning that
        // at the end of everything, we're just adding something to every double in the set by duration?
        const double x = x_o + p.x();
        const double y = y_o + p.y();
#ifdef DEBUG_POINTS
        const Location from_xy{static_cast<Idx>(y), static_cast<Idx>(x)};
        auto seek_cell_pts = r1.map_.find(from_xy);
#endif
        CellPoints& cell_pts = r1.insert(x, y);
#ifdef DEBUG_POINTS
        if (r1.map_.end() != seek_cell_pts)
        {
          logging::check_equal(
            &(seek_cell_pts->second),
            &cell_pts,
            "cell_pts");
        }
        logging::check_fatal(
          r1.unique().empty(),
          "Empty after inserting (%f, %f)",
          x,
          y);
#endif
        const Location& dst = cell_pts.location();
#ifdef DEBUG_POINTS
        logging::check_equal(
          dst.column(),
          from_xy.column(),
          "column");
        logging::check_equal(
          dst.row(),
          from_xy.row(),
          "row");
        logging::check_equal(
          dst.hash(),
          from_xy.hash(),
          "hash");
        CellPoints& cell_pts1 = r1.map_[dst];
        if (r1.map_.end() != seek_cell_pts)
        {
          logging::check_equal(
            &(seek_cell_pts->second),
            &cell_pts1,
            "cell_pts1");
        }
        logging::check_equal(
          cell_pts.cell_x_,
          cell_pts1.cell_x_,
          "cell_x_ lookup");
        logging::check_equal(
          cell_pts.cell_y_,
          cell_pts1.cell_y_,
          "cell_y_ lookup");
        logging::check_equal(
          static_cast<Idx>(x),
          dst.column(),
          "column/x");
        logging::check_equal(
          static_cast<Idx>(y),
          dst.row(),
          "row/y");
#endif
        if (src != dst)
        {
          // we inserted a pair of (src, dst), which means we've never
          // calculated the relativeIndex for this so add it to main map
          cell_pts.add_source(
            relativeIndex(
              src,
              dst));
        }
      }
    }
  }
#ifdef DEBUG_POINTS
  logging::check_fatal(
    r1.map_.empty(),
    "r1.map_.empty()");
  logging::check_fatal(
    offsets.empty(),
    "Applied no offsets");
  logging::check_fatal(
    cell_pts.empty(),
    "Applied offsets to no points");
#endif
  return r1;
}

/**
 * \brief Move constructor
 * \param rhs CellPoints to move from
 */
CellPoints::CellPoints(CellPoints&& rhs) noexcept
  : pts_(std::move(rhs.pts_)),
    cell_x_(rhs.cell_x_),
    cell_y_(rhs.cell_y_),
    src_(rhs.src_)
{
}
/**
 * \brief Copy constructor
 * \param rhs CellPoints to copy from
 */
CellPoints::CellPoints(const CellPoints& rhs) noexcept
  : pts_({}),
    cell_x_(rhs.cell_x_),
    cell_y_(rhs.cell_y_),
    src_(rhs.src_)
{
  std::copy(rhs.pts_.begin(), rhs.pts_.end(), pts_.begin());
}
/**
 * \brief Move assignment
 * \param rhs CellPoints to move from
 * \return This, after assignment
 */
CellPoints& CellPoints::operator=(CellPoints&& rhs) noexcept
{
  pts_ = std::move(rhs.pts_);
  cell_x_ = rhs.cell_x_;
  cell_y_ = rhs.cell_y_;
  src_ = rhs.src_;
  return *this;
}
/**
 * \brief Copy assignment
 * \param rhs CellPoints to copy from
 * \return This, after assignment
 */
CellPoints& CellPoints::operator=(const CellPoints& rhs) noexcept
{
  std::copy(rhs.pts_.begin(), rhs.pts_.end(), pts_.begin());
  cell_x_ = rhs.cell_x_;
  cell_y_ = rhs.cell_y_;
  src_ = rhs.src_;
  return *this;
}
bool CellPoints::operator<(const CellPoints& rhs) const noexcept
{
  if (cell_x_ == rhs.cell_x_)
  {
    if (cell_y_ == rhs.cell_y_)
    {
      for (size_t i = 0; i < pts_.size(); ++i)
      {
        if (pts_[i] != rhs.pts_[i])
        {
          return pts_[i] < rhs.pts_[i];
        }
      }
      // all points are equal if we got here
    }
    return cell_y_ < rhs.cell_y_;
  }
  return cell_x_ < rhs.cell_x_;
}
bool CellPoints::operator==(const CellPoints& rhs) const noexcept
{
  if (cell_x_ == rhs.cell_x_ && cell_y_ == rhs.cell_y_)
  {
    for (size_t i = 0; i < pts_.size(); ++i)
    {
      if (pts_[i] != rhs.pts_[i])
      {
        return false;
      }
    }
    // all points are equal if we got here
    return true;
  }
  return false;
}
bool CellPoints::empty() const
{
  // NOTE: is_invalid() should never be true if it's checking cell_x_
  return unique().empty();
}
bool CellPoints::is_invalid() const
{
#ifdef DEBUG_POINTS
  // if one is invalid then they both should be
  logging::check_equal(
    cell_x_ == INVALID_LOCATION,
    cell_y_ == INVALID_LOCATION,
    "CellPoints Idx is invalid");
  // if invalid then no points should be in list
  if (cell_x_ == INVALID_LOCATION)
  {
    assert_all_invalid(pts_);
  }
#endif
  //   return cell_x_ == INVALID_LOCATION && cell_y_ == INVALID_LOCATION;
  logging::check_fatal(
    cell_x_ == INVALID_LOCATION,
    "CellPoints should always be initialized with some Location");
  return cell_x_ == INVALID_LOCATION;
}
[[nodiscard]] Location CellPoints::location() const noexcept
{
#ifdef DEBUG_POINTS
  logging::check_fatal(
    is_invalid(),
    "Expected cell_x_ and cell_y_ to be set before calling location()");
#endif
  return Location{cell_y_, cell_x_};
}
CellPointsMap::CellPointsMap()
  : map_({})
{
}
void CellPointsMap::emplace(const CellPoints& pts)
{
  const Location location = pts.location();
#ifdef DEBUG_POINTS
  logging::check_equal(
    pts.cell_x_,
    location.column(),
    "pts.cell_x_ to location");
  logging::check_equal(
    pts.cell_y_,
    location.row(),
    "pts.cell_y_ to location");
#endif
  auto e = map_.try_emplace(location, pts);
  CellPoints& cell_pts = e.first->second;
#ifdef DEBUG_POINTS
  logging::check_equal(
    cell_pts.cell_x_,
    location.column(),
    "cell_pts.cell_x_ to location");
  logging::check_equal(
    cell_pts.cell_y_,
    location.row(),
    "cell_pts.cell_y_ to location");
#endif
#ifdef DEBUG_POINTS
  logging::check_equal(
    cell_pts.cell_x_,
    pts.cell_x_,
    "cell_pts.cell_x_ to original");
  logging::check_equal(
    cell_pts.cell_y_,
    pts.cell_y_,
    "cell_pts.cell_y_ to original");
#endif
  if (!e.second)
  {
    // couldn't insert
    cell_pts.merge(pts);
  }
#ifdef DEBUG_POINTS
  CellPoints& cell_pts1 = map_[location];
  logging::check_equal(
    cell_pts.cell_x_,
    cell_pts1.cell_x_,
    "cell_x_ lookup");
  logging::check_equal(
    cell_pts.cell_y_,
    cell_pts1.cell_y_,
    "cell_y_ lookup");
  logging::check_fatal(
    INVALID_LOCATION == cell_pts.cell_x_,
    "CellPoints has invalid cell_x_");
  logging::check_fatal(
    INVALID_LOCATION == cell_pts.cell_y_,
    "CellPoints has invalid cell_y_");
#endif
}
CellPoints& CellPointsMap::insert(const double x, const double y) noexcept
{
  const Location location{static_cast<Idx>(y), static_cast<Idx>(x)};
  auto e = map_.try_emplace(location, x, y);
  CellPoints& cell_pts = e.first->second;
#ifdef DEBUG_POINTS
  logging::check_fatal(
    INVALID_LOCATION == cell_pts.cell_x_,
    "CellPoints has invalid cell_x_");
  logging::check_fatal(
    INVALID_LOCATION == cell_pts.cell_y_,
    "CellPoints has invalid cell_y_");
#endif
  if (!e.second)
  {
    // tried to add new CellPoints but already there
    cell_pts.insert(x, y);
  }
#ifdef DEBUG_POINTS
  logging::check_equal(static_cast<Idx>(x), cell_pts.cell_x_, "cell_x_");
  logging::check_equal(static_cast<Idx>(y), cell_pts.cell_y_, "cell_y_");
  logging::check_equal(location.column(), cell_pts.cell_x_, "cell_x_");
  logging::check_equal(location.row(), cell_pts.cell_y_, "cell_y_");
  logging::check_equal(location.row(), cell_pts.location().row(), "row");
  logging::check_equal(location.column(), cell_pts.location().column(), "column");
#endif
  return e.first->second;
}
CellPointsMap& CellPointsMap::merge(const CellPointsMap& rhs) noexcept
{
  for (const auto& kv : rhs.map_)
  {
#ifdef DEBUG_POINTS
    logging::check_fatal(
      kv.second.is_invalid(),
      "Trying to merge CellPointsMap with invalid CellPoints");
#endif
    emplace(kv.second);
  }
  return *this;
}
void CellPointsMap::remove_if(std::function<bool(const pair<Location, CellPoints>&)> F)
{
  auto it = map_.begin();
#ifdef DEBUG_POINTS
  const auto u0 = unique();
  const auto s0 = u0.size();
  size_t removed = 0;
  logging::check_fatal(
    u0.empty(),
    "Checking removal from empty CellPoints");
#endif
  while (map_.end() != it)
  {
    const Location location = it->first;
#ifdef DEBUG_POINTS
    const CellPoints& cell_pts = it->second;
    const auto u = cell_pts.unique();
    logging::check_fatal(
      u.empty(),
      "Checking if empty CellPoints should be removed");
    logging::check_equal(location.column(), cell_pts.cell_x_, "cell_x_");
    logging::check_equal(location.row(), cell_pts.cell_y_, "cell_y_");
    logging::check_equal(location.row(), cell_pts.location().row(), "row");
    logging::check_equal(location.column(), cell_pts.location().column(), "column");
#endif
    if (F(*it))
    {
      // remove if F returns true for current
      logging::verbose(
        "Removing CellPoints for (%d, %d)",
        location.column(),
        location.row());
      it = map_.erase(it);
#ifdef DEBUG_POINTS
      // if all points from that were in the original then it should be exactly that many fewer
      removed += u.size();
#endif
    }
    else
    {
      ++it;
    }
#ifdef DEBUG_POINTS
    const auto u_cur = unique();
    logging::check_equal(
      u_cur.size(),
      s0 - removed,
      "u_cur.size()");
#endif
  }
}
set<InnerPos> CellPointsMap::unique() const noexcept
{
  set<InnerPos> r{};
  for (const auto& kv : map_)
  {
    const auto u = kv.second.unique();
#ifdef DEBUG_POINTS
    const auto s0 = r.size();
    const auto s1 = u.size();
#endif
    r.insert(u.begin(), u.end());
#ifdef DEBUG_POINTS
    logging::check_fatal(
      r.size() < s1,
      "Less points after insertion: (%ld vs %ld)",
      r.size(),
      s1);
    logging::check_fatal(
      r.size() < s0,
      "Less points than inserted: (%ld vs %ld)",
      r.size(),
      s0);
    logging::check_fatal(
      r.size() > (s0 + s1),
      "More points than possible after insertion: (%ld vs %ld)",
      r.size(),
      (s0 + s1));

#endif
  }
  return r;
}
}
