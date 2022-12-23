# Rubbershifter

A pitch-shifter external for Pure Data using the [RubberBand](https://breakfastquay.com/rubberband/) library.

The **[rubbershifter]** object is a real-time pitch-shifter, trying to minimize the induced audio latency.

Every real-time option of RubberBand is supported, via a dedicated input message:

 - engine faster|finer
 - transients crisp|mixed|smooth
 - detector compound|percussive|soft
 - phase laminar|independent
 - window standard|short|long
 - smoothing off|on
 - formant shifted|preserved
 - priority speed|quality|consistency
 - channel apart|together

-------
## License
Copyright (c) Antoine Rousseau <antoine@metalu.net> 2022  
Standard Improved BSD License.  
For information on usage and redistribution, and for a DISCLAIMER OF ALL WARRANTIES, see the file "LICENSE" in this distribution.

Note that this copyright and license only apply to the current repository, i.e the code and the help patch of the [rubbershifter] Pd external.

Rubber Band Library itself is licensed under the GNU General Public License. If you want to distribute it in a proprietary commercial application, you need to buy a licence. [Read more about this](https://breakfastquay.com/rubberband/license.html).