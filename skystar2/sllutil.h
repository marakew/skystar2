#ifndef _sllutil_h_
#define _sllutil_h_

#define setb_reg(r, o, v) \
        write_reg((r), (o), read_reg((r), (o)) | (v))

extern void
write_reg(struct adapter *sc, u_int32_t reg, u_int32_t val);

extern u_int32_t
read_reg(struct adapter *sc, u_int32_t reg);

#endif
