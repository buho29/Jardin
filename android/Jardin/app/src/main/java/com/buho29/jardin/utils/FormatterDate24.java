package com.buho29.jardin.utils;

public class FormatterDate24 {
    String mPattern;

    public FormatterDate24(String pattern) {
        mPattern = pattern;
    }

    public String format(int date)
    {

        int h, m, s;
        int time = date;

        s = time % 60;
        time /= 60;
        m = time % 60;
        time /= 60;
        h = time % 24;

        return String.format(mPattern,h,m,s);
    }
}
