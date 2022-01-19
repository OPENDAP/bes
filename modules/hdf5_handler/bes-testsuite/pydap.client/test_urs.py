from pydap.client import open_url
from pydap.cas.urs import setup_session
#May choose your own URL to connect NASA Hyrax services
url='https://airsl2.gesdisc.eosdis.nasa.gov/opendap/Aqua_AIRS_Level2/AIRG2SSD.006/2002/243/AIRS.2002.08.31.L2G.Precip_Est.v1.0.3.0.G13208041617.hdf'
#You may also set up the session at your .cshrc or .bashrc.
session=setup_session('your earthdata URS username','your password',check_url=url)
dataset=open_url(url,session=session)
print(dataset)
