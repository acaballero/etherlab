#ifndef TRX_FRONTEND_RECEIVE_UI_H
#define TRX_FRONTEND_RECEIVE_UI_H

class ConsoleWidget;
class WaterfallWidget;
class IQBalanceWidget;

namespace dspReceiveUI {

void init(ConsoleWidget *console, WaterfallWidget *waterfall, IQBalanceWidget *iqbalance);
void before_paint();
void deinit();

} // namespace dspReceiveUI

#endif // TRX_FRONTEND_RECEIVE_UI_H
