def float_str_without_scientific_notion(real):
    fl_str = "{:.15f}".format(real)
    p = f.partition(".")
    # p[2][0] makes sure 0.0 is represented properly
    string = "".join(p[0], p[1], p[2][0], p[2][1:].rstrip("0")) 
    return string
