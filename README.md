bitpay/ubercart7-plugin
=======================

# Installation

Copy these files into `sites/all/modules/` in your Drupal directory.

# Configuration

* Sign up for a merchant account with Bitpay, at https://bitpay.com. Be sure to
  read all provided information thoroughly, and to understand the fees that
  will be charged.
* On your Bitpay merchant page, provide deposit information. This can be
  information for your bank account, or a forwarding bitcoin address, or some
  mixture thereof (you can set the funds to be converted to different
  currencies in differing proportions.)
* Create an API key at https://bitpay.com by clicking My Account > API Access
  Keys > Add New API Key.
* Under Administration > Modules, verify that the Bitpay module is enabled
  under the Ubercart - payment section.
* Under Store > Configuration > Payment Methods, enable the Bitpay payment
  method, and then go to the Bitpay settings menu.
* Enter your API Key under the Administrator settings dropdown menu, and enter
  other settings as desired.
* Select a transaction speed under General settings. The **high** speed will
  send a confirmation as soon as a transaction is received in the bitcoin
  network (usually a few seconds). A **medium** speed setting will typically
  take 10 minutes. The **low** speed setting usually takes around 1 hour. See
  the bitpay.com merchant documentation for a full description of the
  transaction speed settings: https://bitpay.com/downloads/bitpayApi.pdf

# Usage

* When a shopper chooses the Bitcoin payment method, they will be presented
  with an order summary as the next step (prices are shown in whatever
  currency they've selected for shopping). 
* Here, the shopper can either pay to the one-time-use address given, scan the
  QR code to pay, or use the click-to-pay button if they're using an
  URI-compatible wallet. 

**Note:** This extension does not provide a means of automatically pulling a
current BTC exchange rate for presenting BTC prices to shoppers.

# Support

## BitPay Support

* [Github Issues](https://github.com/bitpay/ubercart7-plugin/issues)
  * Open an Issue if you are having issues with this plugin
* [Support](https://support.bitpay.com/)
  * Checkout the BitPay support site
* [Bitpay Labs forum for Ubercart](http://bitpaylabs.com/c/plugins/ubercart)

## Ubercart Support

* [Homepage](http://www.ubercart.org/)
* [Documentation](http://www.ubercart.org/docs)
* [Forums](http://www.ubercart.org/forum)

# Contribute

To contribute to this project, please fork and submit a pull request.

# License

The MIT License (MIT)

Copyright (c) 2011-2015 BitPay

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
