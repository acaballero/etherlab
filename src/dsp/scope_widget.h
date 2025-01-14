//
// Created by Angel Dust on 19/04/2021.
//

#ifndef TRX_FRONTEND_SCOPE_WIDGET_H
#define TRX_FRONTEND_SCOPE_WIDGET_H

#include "../ui/widget.h"
#include "../types.h"

class ScopeWidget : public Widget {
public:

    void paint_callback() override;

protected:
    void before_paint() override;
};

#endif //TRX_FRONTEND_SCOPE_WIDGET_H
