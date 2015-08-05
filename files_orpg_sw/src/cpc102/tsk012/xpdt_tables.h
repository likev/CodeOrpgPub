/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2005/06/02 19:32:53 $
 * $Id: xpdt_tables.h,v 1.2 2005/06/02 19:32:53 ccalvert Exp $
 * $Revision: 1.2 $
 * $State: Exp $
*/


/* Product identifying data */
char *product_name[] = {
              " ",	/* Product   0 */
              " ",	/* Product   1 */
              " ",	/* Product   2 */
              " ",	/* Product   3 */
              " ",	/* Product   4 */
              " ",	/* Product   5 */
              " ",	/* Product   6 */
              " ",	/* Product   7 */
              " ",	/* Product   8 */
              " ",	/* Product   9 */
              " ",	/* Product  10 */
              " ",	/* Product  11 */
              " ",	/* Product  12 */
              " ",	/* Product  13 */
              " ",	/* Product  14 */
              " ",	/* Product  15 */
              "Base Reflectivity       R   16    .54 nm x 1 deg",
              "Base Reflectivity       R   17    1.1 nm x 1 deg",
              "Base Reflectivity       R   18    2.2 nm x 1 deg",
              "Base Reflectivity       R   19    .54 nm x 1 deg",
              "Base Reflectivity       R   20    1.1 nm x 1 deg",
              "Base Reflectivity       R   21    2.2 nm x 1 deg",
              "Base Velocity           V   22    .13 nm x 1 deg",
              "Base Velocity           V   23    .27 nm x 1 deg",
              "Base Velocity           V   24    .54 nm x 1 deg",
              "Base Velocity           V   25    .13 nm x 1 deg",
              "Base Velocity           V   26    .27 nm x 1 deg",
              "Base Velocity           V   27    .54 nm x 1 deg",
              "Base Spectrum Width     SW  28    .13 nm x 1 deg",
              "Base Soectrum Width     SW  29    .27 nm x 1 deg",
              "Base Spectrum Width     SW  30    .54 nm x 1 deg",
              "User Selectable Precip  USP 31    1.1 nm x 1 deg",
              "Digital Hybrid Reflect  DHR 32    .54 nm x 1 deg",
              "Hybrid Scan Reflect     HSR 33    .54 nm x 1 deg",
              "Clutter Filter Control  CFC 34    1.1 nm x 1.4 deg",
              "Composite Reflectivity  CR  35    .54 nm x .54 nm",
              "Composite Reflectivity  CR  36    2.2 nm x 2.2 nm",
              "Composite Reflectivity  CR  37    .54 nm x .54 nm",
              "Composite Reflectivity  CR  38    2.2 nm x 2.2 nm",
              "Comp Ref Contour        CRC 39    .54 nm x .54 nm",
              "Comp Ref Contour        CRC 40    2.2 nm x 2.2 nm",
              "Echo Tops               ET  41    2.2 nm x 2.2 nm",
              "Echo Tops Contour       ETC 42    2.2 nm x 2.2 nm",
              "Severe Wx Reflectivity  SWR 43    .54 nm x 1 deg",
              "Severe Wx Velocity      SWV 44    .13 nm x 1 deg",
              "Severe Wx Width         SWW 45    .13 nm x 1 deg",
              "Severe Wx Shear         SWS 46    .27 nm x 1 deg",
              "Severe Wx Probability   SWP 47    2.2 nm x 2.2 nm",
              "VAD Wind Profile        VWP 48 ",
              "Combined Moment         CM  49    .27 nm x .27 nm",
              "X-Section Reflectivity  RCS 50    .54 nm x .27 nm",
              "X-Section Velocity      VCS 51    .54 nm x .27 nm",
              "X-Section Width         SCS 52    .54 nm x .27 nm",
              "Weak Echo Region        WER 53    .54 nm x .54 nm",
              " ",	/* Product  54 */
              "Storm Relative Region   SRR 55    .27 nm x 1 deg",
              "Storm Relative Map      SRM 56    .54 nm x 1 deg",
              "Vert Integrated Liquid  VIL 57    2.2 nm x 2.2 nm",
              "Storm Tracking Info     STI 58",
              "Hail Index              HI  59",
              "Mesocyclone             M   60",
              "Tornado Vortex Sig      TVS 61",
              "Storm Structure         SS  62",
              "Layer Ref Average L     LRA 63    2.2 nm x 2.2 nm",
              "Layer Ref Average M     LRA 64    2.2 nm x 2.2 nm",
              "Layer Ref Maximum L     LRM 65    2.2 nm x 2.2 nm",
              "Layer Ref Maximum M     LRM 66    2.2 nm x 2.2 nm",
              "Layer Ref AP Removed    APR 67    2.2 nm x 2.2 nm",
              "Layer Turb Average M    LTA 68    2.2 nm x 2.2 nm",
              "Layer Turb Average H    LTA 69    2.2 nm x 2.2 nm",
              "Layer Turb Maximum L    LTM 70    2.2 nm x 2.2 nm",
              "Layer Turb Maximum M    LTM 71    2.2 nm x 2.2 nm",
              "Layer Turb Maximum H    LTM 72    2.2 nm x 2.2 nm",
              "User Alert Message      UAM 73",
              "Radar Coded Message     RCM 74",
              "Free Text Message       FTM 75",
              " ",	/* Product  76 */
              " ",	/* Product  77 */
              "1 Hour Precipitation    OHP 78    1.1 nm x 1 deg",
              "3 Hour Precipitation    THP 79    1.1 nm x 1 deg",
              "Storm Total Rainfall    STP 80    1.1 nm x 1 deg",
              "Digital Precip Array    DPA 81",
              "Supplemental Precip     SPD 82",
              "Radar Coded Message     RCM 83    1/16 LFM",
              "Velocity Azimuth Disp   VAD 84",
              "X-Section Reflectivity  RCS 85    .54 nm x .27 nm",
              "X-Section Velocity      VCS 86    .54 nm x .27 nm",
              "Combined Shear          CS  87",
              "Combined Shear Contour  CSC 88",
              "Layer Ref Average H     LRA 89    2.2 nm x 2.2 nm",
              "Layer Ref Maximum H     LRM 90    2.2 nm x 2.2 nm",
              " ",	/* Product  91 */
              " ",	/* Product  92 */
              "ITWS Dig. Base Velo.    DBV 93     .54 nm x 1 deg",
              "Base Refl. Data Array   RD  94     .54 nm x 1 deg ",
              "Comp. Refl. AP-Edited   CRE 95     .54 nm x .54 nm",
              "Comp. Refl. AP-Edited   CRE 96     2.2 nm x 2.2 nm",
              "Comp. Refl. AP-Edited   CRE 97     .54 nm x .54 nm",
              "Comp. Refl. AP-Edited   CRE 98     2.2 nm x 2.2 nm",
              "Base Velo. Data Array   VD  99     .13 nm x 1 deg",
	      " ",	/* Product 100 */
	      " ",	/* Product 101 */
	      " ",	/* Product 102 */
	      " ",	/* Product 103 */
	      " ",	/* Product 104 */
	      " ",	/* Product 105 */
	      " ",	/* Product 106 */
	      " ",	/* Product 107 */
	      " ",	/* Product 108 */
	      " ",	/* Product 109 */
	      " ",	/* Product 110 */
	      " ",	/* Product 111 */
	      " ",	/* Product 112 */
	      " ",	/* Product 113 */
	      " ",	/* Product 114 */
	      " ",	/* Product 115 */
	      " ",	/* Product 116 */
	      " ",	/* Product 117 */
	      " ",	/* Product 118 */
	      " ",	/* Product 119 */
	      " ",	/* Product 120 */
	      " ",	/* Product 121 */
	      " ",	/* Product 122 */
	      " ",	/* Product 123 */
	      " ",	/* Product 124 */
	      " ",	/* Product 125 */
	      " ",	/* Product 126 */
	      " ",	/* Product 127 */
	      " ",	/* Product 128 */
	      " ",	/* Product 129 */
	      " ",	/* Product 130 */
	      " ",	/* Product 131 */
	      "Clutter Likelihood (Refl) CLR 132  .54 nm x 1 deg",
	      "Clutter Likelihood (Dopl) CLD 133  .54 nm x 1 deg",
	      "High Res Digital VIL      DVL 134  .54 nm x 1 deg",
	      " ",	/* Product 135 */
	      " ",	/* Product 136 */
	      " ",	/* Product 137 */
	      " ",	/* Product 138 */
	      " ",	/* Product 139 */
	      " ",	/* Product 140 */
	      " ",	/* Product 141 */
	      " ",	/* Product 142 */
	      " ",	/* Product 143 */
	      " ",	/* Product 144 */
	      " ",	/* Product 145 */
	      " ",	/* Product 146 */
	      " ",	/* Product 147 */
	      " ",	/* Product 148 */
	      " ",	/* Product 149 */
	      " ",	/* Product 150 */
	      " ",	/* Product 151 */
	      " ",	/* Product 152 */
	      " ",	/* Product 153 */
	      " ",	/* Product 154 */
	      " ",	/* Product 155 */
	      " ",	/* Product 156 */
	      " ",	/* Product 157 */
	      " ",	/* Product 158 */
	      " ",	/* Product 159 */
	      " ",	/* Product 160 */
	      " ",	/* Product 161 */
	      " ",	/* Product 162 */
	      " ",	/* Product 163 */
	      " ",	/* Product 164 */
	      " ",	/* Product 165 */
	      " ",	/* Product 166 */
	      " ",	/* Product 167 */
	      " ",	/* Product 168 */
	      " ",	/* Product 169 */
	      " ",	/* Product 170 */
	      " ",	/* Product 171 */
	      " ",	/* Product 172 */
	      " ",	/* Product 173 */
	      " ",	/* Product 174 */
	      " ",	/* Product 175 */
	      " ",	/* Product 176 */
	      " ",	/* Product 177 */
	      " ",	/* Product 178 */
	      " ",	/* Product 179 */
	      " ",	/* Product 180 */
	      " ",	/* Product 181 */
	      " ",	/* Product 182 */
	      " ",	/* Product 183 */
	      " ",	/* Product 184 */
	      " ",	/* Product 185 */
	      " ",	/* Product 186 */
	      " ",	/* Product 187 */
	      " ",	/* Product 188 */
	      " ",	/* Product 189 */
	      " ",	/* Product 190 */
	      " ",	/* Product 191 */
	      " ",	/* Product 192 */
	      " ",	/* Product 193 */
	      " ",	/* Product 194 */
	      " ",	/* Product 195 */
	      " ",	/* Product 196 */
	      " ",	/* Product 197 */
	      " ",	/* Product 198 */
	      " ",	/* Product 199 */
	      " ",	/* Product 200 */
	      " ",	/* Product 201 */
	      " ",	/* Product 202 */
	      " ",	/* Product 203 */
	      " ",	/* Product 204 */
	      " ",	/* Product 205 */
	      " ",	/* Product 206 */
	      " ",	/* Product 207 */
	      " ",	/* Product 208 */
	      " ",	/* Product 209 */
	      " ",	/* Product 210 */
	      " ",	/* Product 211 */
	      " ",	/* Product 212 */
	      " ",	/* Product 213 */
	      " ",	/* Product 214 */
	      " ",	/* Product 215 */
	      " ",	/* Product 216 */
	      " ",	/* Product 217 */
	      " ",	/* Product 218 */
	      " ",	/* Product 219 */
	      " ",      /* Product 220 */
	      " ",      /* Product 221 */
	      " ",      /* Product 222 */
	      " ",      /* Product 223 */
	      " ",      /* Product 224 */
	      " ",      /* Product 225 */
	      " ",      /* Product 226 */
	      " ",      /* Product 227 */
	      " ",      /* Product 228 */
	      " ",      /* Product 229 */
	      " ",	/* Product 230 */
	      " ",	/* Product 231 */
	      " ",	/* Product 232 */
	      " ",	/* Product 233 */
	      " ",	/* Product 234 */
	      " ",	/* Product 235 */
	      " ",	/* Product 236 */
	      " ",	/* Product 237 */
	      " ",	/* Product 238 */
	      " ",	/* Product 239 */
	      " ", 	/* Product 240 */
	      " ", 	/* Product 241 */
	      " ", 	/* Product 242 */
	      " ", 	/* Product 243 */
	      " ", 	/* Product 244 */
	      " ", 	/* Product 245 */
	      " ", 	/* Product 246 */
	      " ", 	/* Product 247 */
	      " ", 	/* Product 248 */
	      " ", 	/* Product 249 */
	      " ",	/* Product 250 */
	      " ",	/* Product 251 */
	      " ",	/* Product 252 */
	      " ",	/* Product 253 */
	      " ",	/* Product 254 */
	      " ",	/* Product 255 */
	      " ",	/* Product 256 */
	      " ",	/* Product 257 */
	      " ",	/* Product 258 */
	      " ",	/* Product 259 */
	      " ",	/* Product 260 */
	      " ",	/* Product 261 */
	      " ",	/* Product 262 */
	      " ",	/* Product 263 */
	      " ",	/* Product 264 */
	      " ",	/* Product 265 */
	      " ",	/* Product 266 */
	      " ",	/* Product 267 */
	      " ",	/* Product 268 */
	      " ",	/* Product 269 */
	      " ",	/* Product 270 */
	      " ",	/* Product 271 */
	      " ",	/* Product 272 */
	      " ",	/* Product 273 */
	      " ",	/* Product 274 */
	      " ",	/* Product 275 */
	      " ",	/* Product 276 */
	      " ",	/* Product 277 */
	      " ",	/* Product 278 */
	      " ",	/* Product 279 */
	      " ",	/* Product 280 */
	      " ",	/* Product 281 */
	      " ",	/* Product 282 */
	      " ",	/* Product 283 */
	      " ",	/* Product 284 */
	      " ",	/* Product 285 */
	      " ",	/* Product 286 */
	      " ",	/* Product 287 */
	      " ",	/* Product 288 */
	      " ",	/* Product 289 */
	      " ",	/* Product 290 */
	      " ",	/* Product 291 */
	      " ",	/* Product 292 */
	      " ",	/* Product 293 */
	      " ",	/* Product 294 */
	      " ",	/* Product 295 */
	      " ",	/* Product 296 */
	      " ",	/* Product 297 */
	      " ",	/* Product 298 */
	      " ",	/* Product 299 */
	      " "	/* Product 300 */
};

/* Resolution table.  Defined in terms kilometers and degrees. */
float xy_azran_reso[][2] = {
                             { 0.0, 0.0 },
                             { 0.0, 0.0 },
                             { 0.0, 0.0 },
                             { 0.0, 0.0 },
                             { 0.0, 0.0 },
                             { 0.0, 0.0 },
                             { 0.0, 0.0 },
                             { 0.0, 0.0 },
                             { 0.0, 0.0 },
                             { 0.0, 0.0 },
                             { 0.0, 0.0 },
                             { 0.0, 0.0 },
                             { 0.0, 0.0 },
                             { 0.0, 0.0 },
                             { 0.0, 0.0 },
                             { 0.0, 0.0 },
                             { 1.0, 1.0 },  /* Product 16 */
                             { 2.0, 1.0 },  /* Product 17 */
                             { 4.0, 1.0 },  /* Product 18 */
                             { 1.0, 1.0 },  /* Product 19 */
                             { 2.0, 1.0 },  /* Product 20 */
                             { 4.0, 1.0 },  /* Product 21 */
                             { 0.25, 1.0 }, /* Product 22 */
                             { 0.5, 1.0 },  /* Product 23 */
                             { 1.0, 1.0 },  /* Product 24 */
                             { 0.25, 1.0 }, /* Product 25 */
                             { 0.5, 1.0 },  /* Product 26 */
                             { 1.0, 1.0 },  /* Product 27 */
                             { 0.25, 1.0 }, /* Product 28 */
                             { 0.5, 1.0 },  /* Product 29 */
                             { 1.0, 1.0 },  /* Product 30 */
                             { 1.0, 2.0 },  /* Product 31 */
                             { 1.0, 1.0 },  /* Product 32 */
                             { 1.0, 1.0 },  /* Product 33 */
                             { 1.0, 1.4 },  /* Product 34 */
                             { 1.0, 1.0 },  /* Product 35 */
                             { 4.0, 4.0 },  /* Product 36 */
                             { 1.0, 1.0 },  /* Product 37 */
                             { 4.0, 4.0 },  /* Product 38 */
                             { 1.0, 1.0 },  /* Product 39 */
                             { 4.0, 4.0 },  /* Product 40 */
                             { 4.0, 4.0 },  /* Product 41 */
                             { 4.0, 4.0 },  /* Product 42 */
                             { 1.0, 1.0 },  /* Product 43 */
                             { 0.25, 1.0 }, /* Product 44 */
                             { 0.25, 1.0 }, /* Product 45 */
                             { 0.5, 1.0 },  /* Product 46 */
                             { 4.0, 4.0 },  /* Product 47 */
                             { 0.0, 0.0 },  /* Product 48 */
                             { 0.5, 0.5 },  /* Product 49 */
                             { 1.0, 0.5 },  /* Product 50 */
                             { 1.0, 0.5 },  /* Product 51 */
                             { 1.0, 0.5 },  /* Product 52 */
                             { 1.0, 0.5 },  /* Product 53 */
                             { 0.0, 0.0 },
                             { 0.5, 1.0 },  /* Product 55 */
                             { 1.0, 1.0 },  /* Product 56 */
                             { 4.0, 4.0 },  /* Product 57 */
                             { 0.0, 0.0 },
                             { 0.0, 0.0 },
                             { 0.0, 0.0 },
                             { 0.0, 0.0 },
                             { 0.0, 0.0 },
                             { 4.0, 4.0 },  /* Product 63 */
                             { 4.0, 4.0 },  /* Product 64 */
                             { 4.0, 4.0 },  /* Product 65 */
                             { 4.0, 4.0 },  /* Product 66 */
                             { 4.0, 4.0 },  /* Product 67 */
                             { 4.0, 4.0 },  /* Product 68 */
                             { 4.0, 4.0 },  /* Product 69 */
                             { 4.0, 4.0 },  /* Product 70 */
                             { 4.0, 4.0 },  /* Product 71 */
                             { 4.0, 4.0 },  /* Product 72 */
                             { 0.0, 0.0 },  /* Product 73 */
                             { 0.0, 0.0 },  /* Product 74 */
                             { 0.0, 0.0 },  /* Product 75 */
                             { 0.0, 0.0 },  /* Product 76 */
                             { 0.0, 0.0 },  /* Product 77 */
                             { 2.0, 2.0 },  /* Product 78 */
                             { 2.0, 2.0 },  /* Product 79 */
                             { 2.0, 2.0 },  /* Product 80 */
                             { 0.0, 0.0 },  /* Product 81 */
                             { 0.0, 0.0 },  /* Product 82 */
                             { 0.0, 0.0 },  /* Product 83 */
                             { 0.0, 0.0 },  /* Product 84 */
                             { 1.0, 0.5 },  /* Product 85 */
                             { 1.0, 0.5 },  /* Product 86 */
                             { 0.0, 0.0 },  /* Product 87 */
                             { 0.0, 0.0 },  /* Product 88 */
                             { 4.0, 4.0 },  /* Product 89 */
                             { 4.0, 4.0 },  /* Product 90 */
                             { 0.0, 0.0 },  /* Product 91 */
                             { 0.0, 0.0 },  /* Product 92 */
                             { 1.0, 1.0 },  /* Product 93 */
                             { 1.0, 1.0 },  /* Product 94 */
                             { 1.0, 1.0 },  /* Product 95 */
                             { 4.0, 4.0 },  /* Product 96 */
                             { 1.0, 1.0 },  /* Product 97 */
                             { 4.0, 4.0 },  /* Product 98 */
                             { 0.25, 1.0},  /* Product 99 */
			     { 0.0, 0.0 },  /* Product 100*/
			     { 0.0, 0.0 },  /* Product 101*/
			     { 0.0, 0.0 },  /* Product 102*/
			     { 0.0, 0.0 },  /* Product 103*/
			     { 0.0, 0.0 },  /* Product 104*/
			     { 0.0, 0.0 },  /* Product 105*/
			     { 0.0, 0.0 },  /* Product 106*/
			     { 0.0, 0.0 },  /* Product 107*/
			     { 0.0, 0.0 },  /* Product 108*/
			     { 0.0, 0.0 },  /* Product 109*/
			     { 0.0, 0.0 },  /* Product 110*/
			     { 0.0, 0.0 },  /* Product 111*/
			     { 0.0, 0.0 },  /* Product 112*/
			     { 0.0, 0.0 },  /* Product 113*/
			     { 0.0, 0.0 },  /* Product 114*/
			     { 0.0, 0.0 },  /* Product 115*/
			     { 0.0, 0.0 },  /* Product 116*/
			     { 0.0, 0.0 },  /* Product 117*/
			     { 0.0, 0.0 },  /* Product 118*/
			     { 0.0, 0.0 },  /* Product 119*/
			     { 0.0, 0.0 },  /* Product 120*/
			     { 0.0, 0.0 },  /* Product 121*/
			     { 0.0, 0.0 },  /* Product 122*/
			     { 0.0, 0.0 },  /* Product 123*/
			     { 0.0, 0.0 },  /* Product 124*/
			     { 0.0, 0.0 },  /* Product 125*/
			     { 0.0, 0.0 },  /* Product 126*/
			     { 0.0, 0.0 },  /* Product 127*/
			     { 0.0, 0.0 },  /* Product 128*/
			     { 0.0, 0.0 },  /* Product 129*/
			     { 0.0, 0.0 },  /* Product 130*/
			     { 0.0, 0.0 },  /* Product 131*/
			     { 1.0, 1.0 },  /* Product 132*/
			     { 0.25, 1.0 },  /* Product 133*/
			     { 1.0, 1.0 },  /* Product 134*/
			     { 0.0, 0.0 },  /* Product 135*/
			     { 0.0, 0.0 },  /* Product 136*/
			     { 0.0, 0.0 },  /* Product 137*/
			     { 0.0, 0.0 },  /* Product 138*/
			     { 0.0, 0.0 },  /* Product 139*/
			     { 1.0, 1.0 },  /* Product 140*/
			     { 4.0, 4.0 },  /* Product 141*/
			     { 0.0, 0.0 },  /* Product 142*/
			     { 0.0, 0.0 },  /* Product 143*/
			     { 0.0, 0.0 },  /* Product 144*/
			     { 0.0, 0.0 },  /* Product 145*/
			     { 0.0, 0.0 },  /* Product 146*/
			     { 0.0, 0.0 },  /* Product 147*/
			     { 0.0, 0.0 },  /* Product 148*/
			     { 0.0, 0.0 },  /* Product 149*/
			     { 0.0, 0.0 },  /* Product 150*/
			     { 0.0, 0.0 },  /* Product 151*/
			     { 0.0, 0.0 },  /* Product 152*/
			     { 0.0, 0.0 },  /* Product 153*/
			     { 0.0, 0.0 },  /* Product 154*/
			     { 0.0, 0.0 },  /* Product 155*/
			     { 0.0, 0.0 },  /* Product 156*/
			     { 0.0, 0.0 },  /* Product 157*/
			     { 0.0, 0.0 },  /* Product 158*/
			     { 0.0, 0.0 },  /* Product 159*/
			     { 0.0, 0.0 },  /* Product 160*/
			     { 0.0, 0.0 },  /* Product 161*/
			     { 0.0, 0.0 },  /* Product 162*/
			     { 0.0, 0.0 },  /* Product 163*/
			     { 0.0, 0.0 },  /* Product 164*/
			     { 0.0, 0.0 },  /* Product 165*/
			     { 0.0, 0.0 },  /* Product 166*/
			     { 0.0, 0.0 },  /* Product 167*/
			     { 0.0, 0.0 },  /* Product 168*/
			     { 0.0, 0.0 },  /* Product 169*/
			     { 0.0, 0.0 },  /* Product 170*/
			     { 0.0, 0.0 },  /* Product 171*/
			     { 0.0, 0.0 },  /* Product 172*/
			     { 0.0, 0.0 },  /* Product 173*/
			     { 0.0, 0.0 },  /* Product 174*/
			     { 0.0, 0.0 },  /* Product 175*/
			     { 0.0, 0.0 },  /* Product 176*/
			     { 0.0, 0.0 },  /* Product 177*/
			     { 0.0, 0.0 },  /* Product 178*/
			     { 0.0, 0.0 },  /* Product 179*/
			     { 0.0, 0.0 },  /* Product 180*/
			     { 0.0, 0.0 },  /* Product 181*/
			     { 0.0, 0.0 },  /* Product 182*/
			     { 0.0, 0.0 },  /* Product 183*/
			     { 0.0, 0.0 },  /* Product 184*/
			     { 0.0, 0.0 },  /* Product 185*/
			     { 0.0, 0.0 },  /* Product 186*/
			     { 0.0, 0.0 },  /* Product 187*/
			     { 0.0, 0.0 },  /* Product 188*/
			     { 0.0, 0.0 },  /* Product 189*/
			     { 0.0, 0.0 },  /* Product 190*/
			     { 0.0, 0.0 },  /* Product 191*/
			     { 0.0, 0.0 },  /* Product 192*/
			     { 0.0, 0.0 },  /* Product 193*/
			     { 0.0, 0.0 },  /* Product 194*/
			     { 0.0, 0.0 },  /* Product 195*/
			     { 0.0, 0.0 },  /* Product 196*/
			     { 0.0, 0.0 },  /* Product 197*/
			     { 0.0, 0.0 },  /* Product 198*/
			     { 0.0, 0.0 },  /* Product 199*/
			     { 0.0, 0.0 },  /* Product 200*/
			     { 0.0, 0.0 },  /* Product 201*/
			     { 0.0, 0.0 },  /* Product 202*/
			     { 0.0, 0.0 },  /* Product 203*/
			     { 0.0, 0.0 },  /* Product 204*/
			     { 0.0, 0.0 },  /* Product 205*/
			     { 0.0, 0.0 },  /* Product 206*/
			     { 0.0, 0.0 },  /* Product 207*/
			     { 0.0, 0.0 },  /* Product 208*/
			     { 0.0, 0.0 },  /* Product 209*/
			     { 0.0, 0.0 },  /* Product 210*/
			     { 0.0, 0.0 },  /* Product 211*/
			     { 0.0, 0.0 },  /* Product 212*/
			     { 0.0, 0.0 },  /* Product 213*/
			     { 0.0, 0.0 },  /* Product 214*/
			     { 0.0, 0.0 },  /* Product 215*/
			     { 0.0, 0.0 },  /* Product 216*/
			     { 0.0, 0.0 },  /* Product 217*/
			     { 0.0, 0.0 },  /* Product 218*/
			     { 0.0, 0.0 },  /* Product 219*/
			     { 0.0, 0.0 },  /* Product 220*/
			     { 0.0, 0.0 },  /* Product 221*/
			     { 0.0, 0.0 },  /* Product 222*/
			     { 0.0, 0.0 },  /* Product 223*/
			     { 0.0, 0.0 },  /* Product 224*/
			     { 0.0, 0.0 },  /* Product 225*/
			     { 0.0, 0.0 },  /* Product 226*/
			     { 0.0, 0.0 },  /* Product 227*/
			     { 0.0, 0.0 },  /* Product 228*/
			     { 0.0, 0.0 },  /* Product 229*/
			     { 0.0, 0.0 },  /* Product 230*/
			     { 0.0, 0.0 },  /* Product 231*/
			     { 0.0, 0.0 },  /* Product 232*/
			     { 0.0, 0.0 },  /* Product 233*/
			     { 0.0, 0.0 },  /* Product 234*/
			     { 0.0, 0.0 },  /* Product 235*/
			     { 0.0, 0.0 },  /* Product 236*/
			     { 0.0, 0.0 },  /* Product 237*/
			     { 0.0, 0.0 },  /* Product 238*/
			     { 0.0, 0.0 },  /* Product 239*/
			     { 0.0, 0.0 },  /* Product 240*/
			     { 0.0, 0.0 },  /* Product 241*/
			     { 0.0, 0.0 },  /* Product 242*/
			     { 0.0, 0.0 },  /* Product 243*/
			     { 0.0, 0.0 },  /* Product 244*/
			     { 0.0, 0.0 },  /* Product 245*/
			     { 0.0, 0.0 },  /* Product 246*/
			     { 0.0, 0.0 },  /* Product 247*/
			     { 0.0, 0.0 },  /* Product 248*/
			     { 0.0, 0.0 },  /* Product 249*/
			     { 0.0, 0.0 },  /* Product 250*/
			     { 0.0, 0.0 },  /* Product 251*/
			     { 0.0, 0.0 },  /* Product 252*/
			     { 0.0, 0.0 },  /* Product 253*/
			     { 0.0, 0.0 },  /* Product 254*/
			     { 0.0, 0.0 },  /* Product 255*/
			     { 0.0, 0.0 },  /* Product 256*/
			     { 0.0, 0.0 },  /* Product 257*/
			     { 0.0, 0.0 },  /* Product 258*/
			     { 0.0, 0.0 },  /* Product 259*/
			     { 0.0, 0.0 },  /* Product 260*/
			     { 0.0, 0.0 },  /* Product 261*/
			     { 0.0, 0.0 },  /* Product 262*/
			     { 0.0, 0.0 },  /* Product 263*/
			     { 0.0, 0.0 },  /* Product 264*/
			     { 0.0, 0.0 },  /* Product 265*/
			     { 0.0, 0.0 },  /* Product 266*/
			     { 0.0, 0.0 },  /* Product 267*/
			     { 0.0, 0.0 },  /* Product 268*/
			     { 0.0, 0.0 },  /* Product 269*/
			     { 0.0, 0.0 },  /* Product 270*/
			     { 0.0, 0.0 },  /* Product 271*/
			     { 0.0, 0.0 },  /* Product 272*/
			     { 0.0, 0.0 },  /* Product 273*/
			     { 0.0, 0.0 },  /* Product 274*/
			     { 0.0, 0.0 },  /* Product 275*/
			     { 0.0, 0.0 },  /* Product 276*/
			     { 0.0, 0.0 },  /* Product 277*/
			     { 0.0, 0.0 },  /* Product 278*/
			     { 0.0, 0.0 },  /* Product 279*/
			     { 0.0, 0.0 },  /* Product 280*/
			     { 0.0, 0.0 },  /* Product 281*/
			     { 0.0, 0.0 },  /* Product 282*/
			     { 0.0, 0.0 },  /* Product 283*/
			     { 0.0, 0.0 },  /* Product 284*/
			     { 0.0, 0.0 },  /* Product 285*/
			     { 0.0, 0.0 },  /* Product 286*/
			     { 0.0, 0.0 },  /* Product 287*/
			     { 0.0, 0.0 },  /* Product 288*/
			     { 0.0, 0.0 },  /* Product 289*/
			     { 0.0, 0.0 },  /* Product 290*/
			     { 0.0, 0.0 },  /* Product 291*/
			     { 0.0, 0.0 },  /* Product 292*/
			     { 0.0, 0.0 },  /* Product 293*/
			     { 0.0, 0.0 },  /* Product 294*/
			     { 0.0, 0.0 },  /* Product 295*/
			     { 0.0, 0.0 },  /* Product 296*/
			     { 0.0, 0.0 },  /* Product 297*/
			     { 0.0, 0.0 },  /* Product 298*/
			     { 0.0, 0.0 },  /* Product 299*/
			     { 0.0, 0.0 }   /* Product 300*/
};

/* Product color levels table */
int data_level_tab[] = { 0,   0,   0,   0,   0,   0,   0,   0,	/*   0 -   7 */
			 0,   0,   0,   0,   0,   0,   0,   0,  /*   8 -  15 */
                         8,   8,   8,  16,  16,  16,   8,   8,  /*  16 -  23 */
			 8,  16,  16,  16,   8,   8,   8,  16,	/*  24 -  31 */
                       256,  16,   8,   8,   8,  16,  16,   0,  /*  32 -  39 */
			 0,  16,   0,  16,  16,   8,  16,   0,  /*  40 -  47 */
                         6,  16,  16,  16,   8,   8,   0,  16,  /*  48 -  55 */
		        16,  16,   0,   0,   0,   0,   0,   8,	/*  56 -  63 */
                         8,   8,   8,   8,   8,   8,   8,   8,  /*  64 -  71 */
			 8,   0,   9,   0,   0,   0,  16,  16,  /*  72 -  79 */
                        16,   0,   8,   9,   8,   8,   8,  16,  /*  80 -  87 */
			 0,   8,   8,   0,   0, 256, 256,   8,  /*  88 -  95 */
                         8,  16,  16, 256,   0,   0,   0,   0,  /*  96 - 103 */
			 0,   0,   0,   0,   0,   0,   0,   0,  /* 104 - 111 */
			 0,   0,   0,   0,   0,   0,   0,   0,  /* 112 - 119 */
			 0,   0,   0,   0,   0,   0,   0,   0,  /* 120 - 127 */
			 0,   0,   0,   0,  11,  12, 256,   0,  /* 128 - 135 */
			 0,   0,   0,   0,   0,   0,   0,   0,  /* 136 - 143 */
			 0,   0,   0,   0,   0,   0,   0,   0,  /* 144 - 151 */
			 0,   0,   0,   0,   0,   0,   0,   0,  /* 152 - 159 */
			 0,   0,   0,   0,   0,   0,   0,   0,  /* 160 - 167 */
			 0,   0,   0,   0,   0,   0,   0,   0,  /* 168 - 175 */
			 0,   0,   0,   0,   0,   0,   0,   0,  /* 176 - 183 */
			 0,   0,   0,   0,   0,   0,   0,   0,  /* 184 - 191 */
			 0,   0,   0,   0,   0,   0,   0,   0,  /* 192 - 199 */
			 0,   0,   0,   0,   0,   0,   0,   0,  /* 200 - 207 */
			 0,   0,   0,   0,   0,   0,   0,   0,  /* 208 - 215 */
			 0,   0,   0,   0,   0,   0,   0,   0,  /* 216 - 223 */
			 0,   0,   0,   0,   0,   0,   0,   0,  /* 224 - 231 */
			 0,   0,   0,   0,   0,   0,   0,   0,  /* 232 - 239 */
			 0,   0,   0,   0,   0,   0,   0,   0,  /* 240 - 247 */
			 0,   0,   0,   0,   0,   0,   0,   0,  /* 248 - 255 */
			 0,   0,   0,   0,   0,   0,   0,   0,  /* 256 - 263 */
			 0,   0,   0,   0,   0,   0,   0,   0,  /* 264 - 271 */
			 0,   0,   0,   0,   0,   0,   0,   0,  /* 272 - 279 */
			 0,   0,   0,   0,   0,   0,   0,   0,  /* 280 - 287 */
			 0,   0,   0,   0,   0,   0,   0,   0,  /* 288 - 295 */
			 0,   0,   0,   0,   0}; 		/* 296 - 300 */

