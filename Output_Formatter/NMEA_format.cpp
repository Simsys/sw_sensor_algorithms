/***********************************************************************//**
 * @file		NMEA_format.cpp
 * @brief		ASCII converters for NMEA string output
 * @author		Dr. Klaus Schaefer
 * @copyright 		Copyright 2021 Dr. Klaus Schaefer. All rights reserved.
 * @license 		This project is released under the GNU Public License GPL-3.0

    <Larus Flight Sensor Firmware>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

 **************************************************************************/

#include "NMEA_format.h"
#include "embedded_math.h"

#define ANGLE_SCALE 1e-7
#define MPS_TO_NMPH 1.944 // 90 * 60 NM / 10000km * 3600 s/h
#define RAD_TO_DEGREE_10 572.958
#define METER_TO_FEET 3.2808

ROM char HEX[]="0123456789ABCDEF";

//! integer to ASCII returning the string end
char * format_integer( uint32_t value, char *s)
{
  if( value < 10)
    {
    *s++ = value + '0';
    return s;
    }
    else
    {
      s = format_integer( value / 10, s);
      s = format_integer( value % 10, s);
    }
  return s;
}

//! format an integer into ASCII with exactly two digits after the decimal point
char * integer_to_ascii_2_decimals( int32_t number, char *s)
{
  if( number < 0)
    {
      *s++ = '-';
      number = -number;
    }
  s = format_integer( number / 100, s);
  *s++='.';
  s[1]=HEX[number % 10]; // format exact 2 decimals
  number /= 10;
  s[0]=HEX[number % 10];
  s[2]=0;
  return s+2;
}

//! format an integer into ASCII with one decimal
char * integer_to_ascii_1_decimal( int32_t number, char *s)
{
  if( number < 0)
    {
      *s++ = '-';
      number = -number;
    }
  s = format_integer( number / 10, s);
  *s++='.';
  s[0]=HEX[number % 10];
  s[1]=0;
  return s+1;
}

//! basically: kind of strcat returning the pointer to the string-end
inline char *append_string( char *target, const char *source)
{
  while( *source)
      *target++ = *source++;
  *target = 0; // just to be sure :-)
  return target;
}

//! append an angle in ASCII into a NMEA string
char * angle_format ( double angle, char * p, char posc, char negc)
{
  bool pos = angle > 0.0f;
  if (!pos)
    angle = -angle;

  int degree = (int) angle;

  *p++ = degree / 10 + '0';
  *p++ = degree % 10 + '0';

  double minutes = (angle - (double) degree) * 60.0;
  int min = (int) minutes;
  *p++ = min / 10 + '0';
  *p++ = min % 10 + '0';

  *p++ = '.';

  minutes -= min;
  minutes *= 100000;
  min = (int) (minutes + 0.5f);

  p[4] = min % 10 + '0';
  min /= 10;
  p[3] = min % 10 + '0';
  min /= 10;
  p[2] = min % 10 + '0';
  min /= 10;
  p[1] = min % 10 + '0';
  min /= 10;
  p[0] = min % 10 + '0';

  p += 5;

  *p++ = ',';
  *p++ = pos ? posc : negc;
  return p;
}

inline float
sqr (float a)
{
  return a * a;
}
ROM char GPRMC[]="$GPRMC,";

//! NMEA-format time, position, groundspeed, track data
char *format_RMC (const coordinates_t &coordinates, char *p)
{
  p = append_string( p, GPRMC);

  *p++ = (coordinates.hour)   / 10 + '0';
  *p++ = (coordinates.hour)   % 10 + '0';
  *p++ = (coordinates.minute) / 10 + '0';
  *p++ = (coordinates.minute) % 10 + '0';
  *p++ = (coordinates.second) / 10 + '0';
  *p++ = (coordinates.second) % 10 + '0';
  *p++ = '.';
  *p++ = '0';
  *p++ = '0';
  *p++ = ',';
  *p++ = coordinates.sat_fix_type != 0 ? 'A' : 'V';
  *p++ = ',';

  p = angle_format (coordinates.latitude, p, 'N', 'S');
  *p++ = ',';

  p = angle_format (coordinates.longitude, p, 'E', 'W');
  *p++ = ',';

  float value = coordinates.speed_motion * MPS_TO_NMPH;

  unsigned knots = (unsigned)(value * 10.0f + 0.5f);
  *p++ = knots / 1000 + '0';
  knots %= 1000;
  *p++ = knots / 100 + '0';
  knots %= 100;
  *p++ = knots / 10 + '0';
  *p++ = '.';
  *p++ = knots % 10 + '0';
  *p++ = ',';

  float true_track = coordinates.heading_motion;
  if( true_track < 0.0f)
    true_track += 6.2832f;
  int angle_10 = true_track * 10.0 + 0.5;

  *p++ = angle_10 / 1000 + '0';
  angle_10 %= 1000;
  *p++ = angle_10 / 100 + '0';
  angle_10 %= 100;
  *p++ = angle_10 / 10 + '0';
  *p++ = '.';
  *p++ = angle_10 % 10 + '0';

  *p++ = ',';

  *p++ = (coordinates.day) / 10 + '0';
  *p++ = (coordinates.day) % 10 + '0';
  *p++ = (coordinates.month) / 10 + '0';
  *p++ = (coordinates.month) % 10 + '0';
  *p++ = ((coordinates.year)%100) / 10 + '0';
  *p++ = ((coordinates.year)%100) % 10 + '0';

  *p++ = ',';
  *p++ = ',';
  *p++ = ',';
  *p++ = 'A';
  *p++ = 0;

  return p;
}

ROM char GPGGA[]="$GPGGA,";

//! NMEA-format position report, sat number and GEO separation
char *format_GGA( const coordinates_t &coordinates, char *p)
{
  p = append_string( p, GPGGA);

  *p++ = (coordinates.hour)   / 10 + '0';
  *p++ = (coordinates.hour)   % 10 + '0';
  *p++ = (coordinates.minute) / 10 + '0';
  *p++ = (coordinates.minute) % 10 + '0';
  *p++ = (coordinates.second) / 10 + '0';
  *p++ = (coordinates.second) % 10 + '0';
  *p++ = '.';
  *p++ = '0';
  *p++ = '0';
  *p++ = ',';

  p = angle_format (coordinates.latitude, p, 'N', 'S');
  *p++ = ',';

  p = angle_format (coordinates.longitude, p, 'E', 'W');
  *p++ = ',';

  *p++ = coordinates.sat_fix_type  >= 0 ? '1' : '0';
  *p++ = ',';

  *p++ = (coordinates.SATS_number) / 10 + '0';
  *p++ = (coordinates.SATS_number) % 10 + '0';
  *p++ = ',';

  *p++ = '0'; // fake HDOP
  *p++ = '.';
  *p++ = '0';
  *p++ = ',';

  uint32_t altitude_msl_dm = coordinates.position.e[DOWN] * -10.0;
  *p++ = altitude_msl_dm / 10000 + '0';
  altitude_msl_dm %= 10000;
  *p++ = altitude_msl_dm / 1000 + '0';
  altitude_msl_dm %= 1000;
  *p++ = altitude_msl_dm / 100 + '0';
  altitude_msl_dm %= 100;
  *p++ = altitude_msl_dm / 10 + '0';
  *p++ = '.';
  *p++ = altitude_msl_dm % 10 + '0';
  *p++ = ',';
  *p++ = 'M';
  *p++ = ',';

  int32_t geo_sep = coordinates.geo_sep_dm;
  if( geo_sep < 0)
    {
      geo_sep = -geo_sep;
      *p++ = '-';
    }
  *p++ = geo_sep / 1000 + '0';
  geo_sep %= 1000;
  *p++ = geo_sep / 100 + '0';
  geo_sep %= 100;
  *p++ = geo_sep / 10 + '0';
  geo_sep %= 10;
  *p++ = '.';
  *p++ = geo_sep + '0';
  *p++ = ',';
  *p++ = 'm';
  *p++ = ','; // no DGPS
  *p++ = ',';
  *p++ = 0;

  return p;
}

ROM char GPMWV[]="$GPMWV,";

//! format wind reporting NMEA sequence
char *format_MWV ( float wind_north, float wind_east, char *p)
{
  p = append_string( p, GPMWV);

//  wind_north = 3.0; // this setting reports 18km/h from 53 degrees
//  wind_east = 4.0;

  while (*p)
    ++p;

  float direction = ATAN2( -wind_east, -wind_north);

  int angle_10 = direction * RAD_TO_DEGREE_10 + 0.5;
  if( angle_10 < 0)
    angle_10 += 3600;

  *p++ = angle_10 / 1000 + '0';
  angle_10 %= 1000;
  *p++ = angle_10 / 100 + '0';
  angle_10 %= 100;
  *p++ = angle_10 / 10 + '0';
  *p++ = '.';
  *p++ = angle_10 % 10 + '0';
  *p++ = ',';
  *p++ = 'T'; // true direction
  *p++ = ',';

  float value = SQRT(sqr( wind_north) + sqr( wind_east));

  unsigned wind = value * 10.0f;
  *p++ = wind / 1000 + '0';
  wind %= 1000;
  *p++ = wind / 100 + '0';
  wind %= 100;
  *p++ = wind / 10 + '0';
  *p++ = '.';
  *p++ = wind % 10 + '0';

  *p++ = ',';
  *p++ = 'M'; // m/s
  *p++ = ',';
  *p++ = 'A'; // valid
  *p++ = 0;

  return p;
}

ROM char POV[]="$POV";

//! format the OpenVario sequence TAS, pressures and TEK variometer
void format_POV( float TAS, float pabs, float pitot, float TEK_vario, float voltage,
		 bool airdata_available, float humidity, float temperature, char *p)
{
  p = append_string( p, POV);

  p = append_string( p, ",E,");
  p = integer_to_ascii_2_decimals( (int)(TEK_vario * 100.0f), p);

  p = append_string( p, ",P,");
  p = integer_to_ascii_2_decimals( (int)pabs, p); // static pressure, already in Pa = 100 hPa

  if( pitot < 0.0f)
    pitot = 0.0f;
  p = append_string( p, ",R,");
  p = integer_to_ascii_2_decimals( (int)pitot, p); // pitot pressure (difference) / Pa

  p = append_string( p, ",S,");
  p = integer_to_ascii_2_decimals( (int)(TAS * 360.0f), p); // m/s -> 1/100 km/h

  p = append_string( p, ",V,");
  p = integer_to_ascii_1_decimal( (int)(voltage * 10.0f), p);

  if( airdata_available)
    {
      p = append_string( p, ",H,");
      p = integer_to_ascii_2_decimals( (int)(humidity * 100.0f), p);

      p = append_string( p, ",T,");
      p = integer_to_ascii_2_decimals( (int)(temperature * 100.0f), p);
    }

  *p = 0;
}

//! add the elements reporting attitude (roll nick yaw angles)
void format_POV_RNY( float roll, float nick, float yaw, char *p)
{
  p = append_string( p, POV);
  p = append_string( p, ",B,"); // bank instead of roll, as "R" is already in use
  p = integer_to_ascii_1_decimal( (int)(roll * RAD_TO_DEGREE_10 + 0.5), p);

  p = append_string( p, ",N,");
  p = integer_to_ascii_1_decimal( (int)(nick * RAD_TO_DEGREE_10 + 0.5), p);

  if( yaw < 0.0f)
    yaw += 6.2832f;
  p = append_string( p, ",Y,");
  p = integer_to_ascii_1_decimal( (int)(yaw * RAD_TO_DEGREE_10 + 0.5), p);

  *p = 0;
}

ROM char HCHDT[]="$HCHDT,";

//! create HCHDM sentence to report true heading
void format_HCHDT( float true_heading, char *p) // report magnetic heading
{
  int heading = (int)(true_heading * 573.0f); // -> 1/10 degree
  if( heading < 0)
    heading += 3600;

  p = append_string( p, HCHDT);
  p = integer_to_ascii_1_decimal( heading, p);
  *p++ = ',';
  *p++ = 'T';

  *p = 0;
}

inline char hex4( uint8_t data)
{
  return HEX[data];
}

//! test a line for valid NMEA checksum
bool NMEA_checksum( const char *line)
 {
 	uint8_t checksum = 0;
 	if( line[0]!='$')
 		return false;
 	const char * p;
 	for( p=line+1; *p && *p !='*'; ++p)
 		checksum ^= *p;
 	return ( (p[0] == '*') && hex4( checksum >> 4) == p[1]) && ( hex4( checksum & 0x0f) == p[2]) && (p[3] == 0);
 }

//! add end delimiter, evaluate and add checksum and add CR+LF
char * NMEA_append_tail( char *p)
 {
 	uint8_t checksum = 0;
 	if( p[0]!='$')
 		return 0;
 	for( p=p+1; *p && *p !='*'; ++p)
 		checksum ^= *p;
 	p[0] = '*';
 	p[1] = hex4(checksum >> 4);
 	p[2] = hex4(checksum & 0x0f);
 	p[3] = '\r';
 	p[4] = '\n';
 	p[5] = 0;
 	return p+5;
 }

//! this procedure formats all our NMEA sequences
void format_NMEA_string( const output_data_t &output_data, string_buffer_t &NMEA_buf, float declination)
{
  char *next;

  format_RMC ( output_data.c, NMEA_buf.string);
  next = NMEA_append_tail (NMEA_buf.string);

  format_GGA ( output_data.c, next);  //TODO: ensure that this reports the altitude in meter above medium sea level and height above wgs84: http://aprs.gids.nl/nmea/#gga
  next = NMEA_append_tail (next);

  format_MWV (output_data.wind_average.e[NORTH], output_data.wind_average.e[EAST], next);
  next = NMEA_append_tail (next);

  format_POV( output_data.TAS, output_data.m.static_pressure, output_data.m.pitot_pressure, output_data.vario, output_data.m.supply_voltage,
	      (output_data.m.outside_air_humidity > 0.0f), // true if outside air data are available
	      output_data.m.outside_air_humidity*100.0f, output_data.m.outside_air_temperature, next);
  next = NMEA_append_tail (next);

  format_POV_RNY( output_data.euler.r, output_data.euler.n, output_data.euler.y, next);
  next = NMEA_append_tail (next);

  format_HCHDT( output_data.euler.y, next); // report magnetic heading
  next = NMEA_append_tail (next);

  NMEA_buf.length = next - NMEA_buf.string;
}

