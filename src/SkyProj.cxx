/** @file SkyProj.cxx
@brief implementation of the class SkyProj

$Header: /nfs/slac/g/glast/ground/cvs/astro/src/SkyProj.cxx,v 1.13 2005/07/08 22:47:45 hierath Exp $
*/

// Include files

#include "tip/IFileSvc.h"
#include "tip/Image.h"
#include "tip/Header.h"
#include "astro/SkyProj.h"
#include "wcslib/wcs.h"

using namespace astro;
#include <string>
#include <sstream>
#include <stdexcept>
#include <cmath>
#include <cassert>

SkyProj::SkyProj(const std::string &projName, 
                 double* crpix, double* crval, double* cdelt, double crota2 ,bool galactic)
{
	double lonpole = 999;
	double latpole = 999;

	SkyProj::init(projName,crpix,crval,cdelt,lonpole,latpole,crota2,galactic);
}

SkyProj::SkyProj(const std::string &projName, 
        double* crpix, double* crval, double* cdelt, double lonpole, double latpole,
		double crota2, bool galactic)
{
	SkyProj::init(projName,crpix,crval,cdelt,lonpole,latpole,crota2,galactic);
}

SkyProj::SkyProj(const std::string & fitsFile, const std::string & extension) {
   const tip::Image * image = 
      tip::IFileSvc::instance().readImage(fitsFile, extension);

   const tip::Header & header = image->getHeader();

   bool galactic;
   std::string ctype;
   header["CTYPE1"].get(ctype);
   if (ctype.substr(0, 2) == "RA") {
      galactic = false;
   } else if (ctype.substr(0, 4) == "GLON") {
      galactic = true;
   } else {
      throw std::runtime_error("Unrecognized coordinate system in " 
                               + fitsFile + "[" + extension + "]");
   }
   
   std::string projName("");
   if (ctype.size() > 7) {
      projName = ctype.substr(ctype.size() - 3, 3);
   }
   
   double crpix[2], crval[2], cdelt[2];
   header["CRPIX1"].get(crpix[0]);
   header["CRVAL1"].get(crval[0]);
   header["CDELT1"].get(cdelt[0]);

   header["CRPIX2"].get(crpix[1]);
   header["CRVAL2"].get(crval[1]);
   header["CDELT2"].get(cdelt[1]);

   double lonpole;
   double latpole;
   try {
      header["LONPOLE"].get(lonpole);
      header["LATPOLE"].get(latpole);
   } catch (tip::TipException &) {
      lonpole = 999;
      latpole = 999;
   }

   double crota2(0);
   try {
      header["CROTA2"].get(crota2);
   } catch (tip::TipException &) {
   }

   delete image;

   init(projName, crpix, crval, cdelt, lonpole, latpole, crota2, galactic);
}


SkyProj::~SkyProj()
{
    wcsfree(m_wcs);
}


/** @brief Do the projection to pixels with the given coordinates
@param s1 ra or l, in degrees
@param s2 dec or b, in degrees
@return pair(x,y) in pixel coordinates
*/
std::pair<double,double> SkyProj::sph2pix(double s1, double s2) const
{
    int ncoords = 1;
    int nelem = 2;
    double  imgcrd[2], pixcrd[2];
    double phi[1], theta[1];
    int stat[1];

    // WCS projection routines require the input coordinates are in degrees
    // and in the range of [-90,90] for the lat and [-180,180] for the lon.
    // So correct for this effect.
    if(s1 > 180) s1 -= 360.;

    double worldcrd[] ={s1,s2};

    int returncode = wcss2p(m_wcs, ncoords, nelem, worldcrd, phi, theta, imgcrd, pixcrd, stat);
    if ( returncode != 0 ) throw SkyProj::Exception(returncode);

    return std::make_pair(pixcrd[0],pixcrd[1]);
}

std::pair<double,double> SkyProj::pix2sph(double x1, double x2) const
{
    int ncoords = 1;
    int nelem = 2;
    double worldcrd[2], imgcrd[2];
    double phi[1], theta[1];
    int stat[1];

    double pixcrd[] = {x1,x2};;

    int returncode = wcsp2s(m_wcs, ncoords, nelem, pixcrd, imgcrd, phi, theta, worldcrd, stat);
    if ( returncode != 0 ) throw SkyProj::Exception(returncode);

    double s1 = worldcrd[0];

    //fold RA into the range [0,360)
    while(s1 < 0) s1 +=360.;
    while(s1 >= 360) s1 -= 360.;

    return std::make_pair<double,double>(s1,worldcrd[1]);
}


/** @brief Convert from one projection to another
@param x1 projected equivalent to ra or l, in degrees
@param x2 projected equivalent dec or b, in degrees
@param projection used to deproject these coordinates
*/
std::pair<double,double> SkyProj::pix2pix(double x1, double x2, const SkyProj& otherProjection)const
{
    std::pair<double,double> s = otherProjection.pix2sph(x1,x2);
    return SkyProj::sph2pix(s.first,s.second);
}

void SkyProj::setLonpole(double lonpole)
{
   m_wcs->lonpole = lonpole;

   int status = wcsset2(m_wcs);
   if (status !=0) {
      throw SkyProj::Exception(status);
   }
}

void SkyProj::setLatpole(double latpole)
{
   m_wcs->latpole = latpole;

   int status = wcsset2(m_wcs);
   if (status !=0) {
      throw SkyProj::Exception(status);
   }
}

bool SkyProj::isGalactic()const
{
    return ( std::string( m_wcs->ctype[0] ).substr(0,4)=="GLON");
};

void SkyProj::init(const std::string &projName, 
                 double* crpix, double* crval, double* cdelt, 
				 double lonpole, double latpole, double crota2, bool galactic)
{
    assert( sizeof_wcslib>=sizeof(wcsprm));
    m_wcs = reinterpret_cast<wcsprm*>(m_wcs_struct);
    m_wcs->flag = -1;

    int naxis = 2;
    wcsini(1, naxis, m_wcs);

    std::string 
        lon_type = (galactic? "GLON-" : "RA---") + projName,
        lat_type =  (galactic? "GLAT-" : "DEC--") + projName;
    strcpy(m_wcs->ctype[0], lon_type.c_str() );
    strcpy(m_wcs->ctype[1], lat_type.c_str() );

    // copy  intput arrays
    for( int i=0; i<naxis; ++i){
        m_wcs->crval[i] = crval[i];  // reference value
        m_wcs->crpix[i] = crpix[i]; // pixel coordinate
        m_wcs->cdelt[i] = cdelt[i]; // scale factor
    }

	if(latpole != 999)
		m_wcs->latpole = latpole;
	if(lonpole != 999)
		m_wcs->lonpole = lonpole;


    // Set wcs to use CROTA rotations instead of PC or CD  transformations
    m_wcs->altlin |= 4;
    m_wcs->crota[1] = crota2;

    int status = wcsset2(m_wcs);
    if (status !=0) {
        throw SkyProj::Exception(status );
    }
    // a simple test
    double tlon = crval[0], tlat = crval[1];
    std::pair<double, double> t = sph2pix(tlon, tlat);
    double check = fabs(t.first-crpix[0])+fabs(t.second-crpix[1]);
    std::pair<double, double> s = pix2sph(t.first, t.second);
    check = fabs(s.first-crval[0]-s.second-crval[1]);

    // enable this to see a nice formatted dump
    //wcsprt(m_wcs);
}

SkyProj::SkyProj(const SkyProj & other)
{
    // copy constructor just resets pointer
    assert( sizeof_wcslib>=sizeof(wcsprm));
    m_wcs = reinterpret_cast<wcsprm*>(m_wcs_struct);    
}

const char * SkyProj::Exception::what() const throw() {
   std::stringstream msg; 
   msg << "SkyProj wcslib error "<< m_status << " : ";
   if(  m_status<1 || m_status>11 ) msg << " unknown error";
   else msg << wcs_errmsg[m_status];
   static char  buf[80];
   ::strncpy(buf, msg.str().c_str(), sizeof(buf));
   return buf;
}
