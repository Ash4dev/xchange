#pragma once

/* references:
 * https://chatgpt.com/share/67e52253-fee0-800c-9434-01439719d181
 * https://www.investor.gov/introduction-investing/general-resources/news-alerts/alerts-bulletins/investor-bulletins-14
 * https://www.5paisa.com/stock-market-guide/stock-share-market/good-till-cancelled-gtc
 */

/* reductions:
 * GoodTillCancel, GoodTillDate, GoodAfterTime, ImmediateOrCancel, Market

 * GoodTillCancel: GoodTillDate w/ max 90 days validity
 * AllOrNone: GoodTillCancel w/ check for full quantity
 * GoodForDay: GoodTillDate w/ today as the date
 * MarketOnOpen: GoodAfterTime (today close) && ImmediateOrCancel
 * MarketOnClose: GoodAfterTime (today close-1) && ImmediateOrCancel
 * FillOrKill: ImmediateOrCancel w/ check for full quantity
 */

// sorted: duration of existence in order book
namespace OrderType {
enum OrderType {
  AllOrNone,
  GoodTillCancel, // stay till partial
  GoodTillDate,   // till future date close time
  GoodForDay,
  GoodAfterTime, // triggered after time on same day
  MarketOnOpen,
  MarketOnClose,
  ImmediateOrCancel, // partial exec at once, then cancel
  FillOrKill,
  Market // rest all are limit orders
};
}

/*
 * market order
 * match as much qty preferentially as possible
 * what cannot be matched (my > other side total)
 * stays as a good till cancel order for at worst price
 * */
