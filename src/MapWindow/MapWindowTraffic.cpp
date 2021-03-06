/*
Copyright_License {

  Top Hat Soaring Glide Computer - http://www.tophatsoaring.org/
  Copyright (C) 2000-2016 The Top Hat Soaring Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "MapWindow.hpp"
#include "Screen/Icon.hpp"
#include "Screen/Layout.hpp"
#include "Formatter/UserUnits.hpp"
#include "Look/TrafficLook.hpp"
#include "Renderer/TextInBox.hpp"
#include "Renderer/TrafficRenderer.hpp"
#include "FLARM/Friends.hpp"
#include "Tracking/SkyLines/Data.hpp"

/**
 * Draws the FLARM traffic icons onto the given canvas
 * @param canvas Canvas for drawing
 */
void
MapWindow::DrawFLARMTraffic(Canvas &canvas,
                            const RasterPoint aircraft_pos) const
{
  // Return if FLARM icons on moving map are disabled
  if (!GetMapSettings().show_flarm_on_map)
    return;

  // Return if FLARM data is not available
  const TrafficList &flarm = Basic().flarm.traffic;
  if (flarm.IsEmpty())
    return;

  const WindowProjection &projection = render_projection;

  // if zoomed in too far out, dont draw traffic since it will be too close to
  // the glider and so will be meaningless (serves only to clutter, cant help
  // the pilot)
  if (projection.GetMapScale() > fixed(7300))
    return;

  canvas.Select(*traffic_look.font);

  // Circle through the FLARM targets
  for (auto it = flarm.list.begin(), end = flarm.list.end();
      it != end; ++it) {
    const FlarmTraffic &traffic = *it;

    if (!traffic.location_available)
      continue;

    // Save the location of the FLARM target
    GeoPoint target_loc = traffic.location;

    // Points for the screen coordinates for the icon, name and average climb
    RasterPoint sc, sc_name, sc_av;

    // If FLARM target not on the screen, move to the next one
    if (!projection.GeoToScreenIfVisible(target_loc, sc))
      continue;

    // Draw the name 16 points below the icon
    sc_name = sc;
    sc_name.y -= Layout::Scale(20);

    // Draw the average climb value above the icon
    sc_av = sc;
    sc_av.y += Layout::Scale(5);

    TextInBoxMode mode;
    mode.shape = LabelShape::OUTLINED;

    // JMW TODO enhancement: decluttering of FLARM altitudes (sort by max lift)

    int dx = sc_av.x - aircraft_pos.x;
    int dy = sc_av.y - aircraft_pos.y;

    // only draw labels if not close to aircraft
    if (dx * dx + dy * dy > Layout::Scale(30 * 30)) {
      // If FLARM callsign/name available draw it to the canvas
      if (traffic.HasName() && !StringIsEmpty(traffic.name))
        TextInBox(canvas, traffic.name, sc_name.x, sc_name.y,
                  mode, GetClientRect());

      if (traffic.climb_rate_avg30s >= fixed(0.1)) {
        // If average climb data available draw it to the canvas
        TCHAR label_avg[100];
        FormatUserVerticalSpeed(traffic.climb_rate_avg30s,
                                       label_avg, false);
        TextInBox(canvas, label_avg, sc_av.x, sc_av.y, mode, GetClientRect());
      }
    }

    auto color = FlarmFriends::GetFriendColor(traffic.id);
    TrafficRenderer::Draw(canvas, traffic_look, traffic,
                          traffic.track - projection.GetScreenAngle(),
                          color, sc);
  }
}


/**
 * Draws the GliderLink traffic icons onto the given canvas
 * @param canvas Canvas for drawing
 */
void
MapWindow::DrawGLinkTraffic(Canvas &canvas,
                            const RasterPoint aircraft_pos) const
{
  // Return if FLARM icons on moving map are disabled
  if (!GetMapSettings().show_flarm_on_map)
    return;

  const GliderLinkTrafficList &traffic = Basic().glink_data.traffic;
  if (traffic.IsEmpty())
    return;

  const MoreData basic = Basic();

  const WindowProjection &projection = render_projection;

  canvas.Select(*traffic_look.font);

  // Circle through the GliderLink targets
  for (auto it = traffic.list.begin(), end = traffic.list.end();
      it != end; ++it) {
    const GliderLinkTraffic &traffic = *it;

    // Save the location of the target
    GeoPoint target_loc = traffic.location;

    // Points for the screen coordinates for the icon, name and average climb
    RasterPoint sc, sc_name, sc_av, sc_alt;

    // If FLARM target not on the screen, move to the next one
    if (!projection.GeoToScreenIfVisible(target_loc, sc))
      continue;

    // Draw the name 16 points below the icon
    sc_name = sc;
    sc_name.x -= Layout::Scale(20);
    sc_name.y -= Layout::Scale(18);

    // Draw the average climb value above the icon
    sc_av = sc;
    sc_av.x += Layout::Scale(5);
    sc_av.y -= Layout::Scale(10);

    // Location of altitude label
    sc_alt = sc;
    sc_alt.x -= Layout::Scale(35);
    sc_alt.y -= Layout::Scale(2);

    TextInBoxMode mode;
    mode.shape = LabelShape::OUTLINED;

    // If callsign/name available draw it to the canvas
    if (traffic.HasName() && !StringIsEmpty(traffic.name))
      TextInBox(canvas, traffic.name, sc_name.x, sc_name.y,
                mode, GetClientRect());

    if (traffic.climb_rate_received) {
      // If average climb data available draw it to the canvas
      TCHAR label_avg[100];
      FormatUserVerticalSpeed(traffic.climb_rate,
                                     label_avg, false);
      TextInBox(canvas, label_avg, sc_av.x, sc_av.y, mode, GetClientRect());
    }

    // use GPS altitude to be consistent with GliderLink
    if(basic.gps_altitude_available && traffic.altitude_available
        && fabs(fixed(traffic.altitude) - basic.gps_altitude) >= fixed(100)) {
      // If average climb data available draw it to the canvas
      TCHAR label_alt[100];
      fixed alt = fixed((fixed(traffic.altitude) - basic.gps_altitude) / 100);
      FormatRelativeUserAltitude(alt, label_alt, false);

      if (alt < fixed(10))
        sc_alt.x += Layout::Scale(10);
      TextInBox(canvas, label_alt, sc_alt.x, sc_alt.y, mode, GetClientRect());
    }

    TrafficRenderer::Draw(canvas, traffic_look, traffic,
                          traffic.track - projection.GetScreenAngle(), sc);
  }
}

/**
 * Draws the teammate icon to the given canvas
 * @param canvas Canvas for drawing
 */
void
MapWindow::DrawTeammate(Canvas &canvas) const
{
  const TeamInfo &teamcode_info = Calculated();

  if (teamcode_info.teammate_available) {
    RasterPoint sc;
    if (render_projection.GeoToScreenIfVisible(teamcode_info.teammate_location,
                                                 sc))
      traffic_look.teammate_icon.Draw(canvas, sc);
  }
}

#ifdef HAVE_SKYLINES_TRACKING_HANDLER

void
MapWindow::DrawSkyLinesTraffic(Canvas &canvas) const
{
  if (skylines_data == nullptr)
    return;

  canvas.Select(*traffic_look.font);

  ScopeLock protect(skylines_data->mutex);
  for (auto &i : skylines_data->traffic) {
    RasterPoint pt;
    if (render_projection.GeoToScreenIfVisible(i.second.location, pt)) {
      traffic_look.teammate_icon.Draw(canvas, pt);
    }
  }
}

#endif
