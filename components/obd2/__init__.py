
def dtc_name_to_int(dtc_name) -> int:
    cats = ['P', 'C', 'B', 'U']
    if len(dtc_name) != 5 or dtc_name[0] not in cats:
        return None

    cat_value = cats.index(dtc_name[0])
    try:
        dtc_code = int(dtc_name[1:], 16)
    except:
        return None

    return (cat_value << 14) | dtc_code


def validate_dtc_name_(value):
    dtc_value = dtc_name_to_int(value)
    if dtc_value is None:
        raise cv.Invalid("Invalid DTC name")
    return value
