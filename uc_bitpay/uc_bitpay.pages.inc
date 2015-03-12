<?php
/**
 * The MIT License (MIT)
 * 
 * Copyright (c) 2011-2015 BitPay
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/**
 * @file
 * Bitpay administration menu items.
 *
 * @creation October 2012
 * @author Rod Jackson (RDJ) - with the bulk of code derived
 * from Ubercart's v.6 included payment modules and from
 * Mike Gogulski's uc_bitcoin payment module
 */
require_once("bp_lib.php");

/**
 * Process Invoice Status Notifiations from Bitpay.
 */
function uc_bitpay_notification()
{
    // DEBUG CODE
    //variable_set('uc_bitpay_notify_check', time());

    // Use bpVerifyNotification to verify the source is authentic, and to
    // convert the $_POST data to an array containing the invoice data.
    $checkerr = 'authentication failed';
    $notify   = bpVerifyNotification(_get_bp_key());

    if (is_string($notify))
    {
        // we have an error; check to see if it's because of a bad API key
        if (substr($notify, 0, strlen($checkerr)) == $checkerr)
        {
            // try our prior API key, on the off chance we changed API keys
            // while one or more invoices were still active
            $notify = bpVerifyNotification(_get_prior_bp_key());
        }
    }		

    // if we received a string instead of an array, we have an error
    if (is_string($notify))
    {
        // if it's due to a bad API key, alert the notification email address
        if (substr($notify, 0, strlen($checkerr)) == $checkerr)
        {
            // send the email
            $to      = variable_get('uc_bitpay_alert_email', '');
            $success = drupal_mail('uc_bitpay', 'invalid_api_key', $to, $language, array(), "BitPay");
        }

        return;
    }

    // get the order ID from our database;
    // if none found, nothing we can do with it
    $order_id = db_query("SELECT order_id FROM {uc_payment_bitpay} WHERE invoice_id = :invoice_id", array(':invoice_id' => $notify['id']))->fetchField();
    if (($order_id === false) || !is_numeric($order_id) || ($order_id == 0))
    {
        return;
    }
    // pull the order status and user ID from the database
    $order_status = db_query("SELECT order_status FROM {uc_orders} WHERE order_id = :order_id", array(':order_id' => $order_id))->fetchField();
    $uid          = db_query("SELECT uid FROM {uc_orders} WHERE order_id = :order_id", array(':order_id' => $order_id))->fetchField();

    // on certain invoice status changes, do certain things
    switch ($notify['status'])
    {
    // PAID: Normally this would reflect the fact that the
    // invoice has been updated from 'new' to 'paid', and the
    // payment address has been sent the full amount requested.
    // This module waits until 'confirmed', 'complete',
    // 'expired' or 'invalid' for any DB updates; it does
    // nothing significant if the invoice is merely 'paid'.
    case 'paid':
        // just save a comment
        uc_order_comment_save($order_id, 0, t("Customer has sent the bitcoin transaction for payment, but it has not confirmed yet."), 'admin');

        // if we're copying notification emails, here's where we do one
        if (variable_get('uc_bitpay_copy_notify_emails', FALSE))
        {
            // construct an alert to email
            $params             = array();
            $params['id']       = $notify['id'];
            $params['url']      = $notify['url'];
            $params['order_id'] = $order_id;
            // send the email
            $to      = variable_get('uc_bitpay_alert_email', '');
            $success = drupal_mail('uc_bitpay', 'paid', $to, $language, $params, "BitPay");
        }

        break;

    // CONFIRMED: Update the DB to reflect the fact that the
    // invoice has been updated to 'confirmed', either from
    // 'new' or from 'paid'. The transaction speed determines
    // how soon 'confirmed' occurs: 'high' will yield 'confirmed'
    // as soon as full payment is made (and will bypass the
    // 'paid' status); 'medium' will yield 'confirmed' after the
    // invoice is 'paid', and the transaction receives one
    // confirmation on the bitcoin blockchain; 'low' will yield
    // 'confirmed' after the invoice is 'paid' and the transaction
    // receives a full six confirmations on the blockchain.
    case 'confirmed':		  
        // mark the order as Payment received
        $state = uc_order_status_data($order_status, 'state');
        if (($state != 'canceled') && ($state != 'completed'))
        {
            uc_order_update_status($order_id, 'payment_received');
        }

        // mark the payment
        uc_payment_enter($order_id, 'bitpay', $notify['price'], $uid, NULL, '', REQUEST_TIME);

        // note the payment confirmation
        uc_order_comment_save($order_id, 0, t("Customer's bitcoin payment has confirmed according to the transaction speed you have set for Bitpay."), 'admin');

        // if we're copying notification emails, here's where we do one
        if (variable_get('uc_bitpay_copy_notify_emails', FALSE))
        {
            // construct an alert to email
            $params             = array();
            $params['id']       = $notify['id'];
            $params['url']      = $notify['url'];
            $params['order_id'] = $order_id;
            // send the email
            $to      = variable_get('uc_bitpay_alert_email', '');
            $success = drupal_mail('uc_bitpay', 'confirmed', $to, $language, $params, "BitPay");
        }
        break;

    // COMPLETE: This status reflects the fact that full payment
    // has been made, that the transaction has received a full six
    // confirmations on the bitcoin blockchain, and that Bitpay has
    // subsequently credited the merchant's account for the invoice.
    // If the transaction speed has been set to 'low', then the
    // status will go from 'paid' to 'complete', bypassing 'confirmed.'
    // This module takes no action upon this status change. Please
    // note that this means this module is not capable of either
    // discovering or resolving any issues resulting from 'high'-
    // speed invoices encountering a double-spend attack.
    case 'complete':
        // mark the order as Payment received if it hasn't been already
        if ($order_status != 'payment_received')
        {
            $state = uc_order_status_data($order_status, 'state');
            if (($state != 'canceled') && ($state != 'completed'))
            {
                uc_order_update_status($order_id, 'payment_received');
            }
            // mark the payment
            uc_payment_enter($order_id, 'bitpay', $notify['price'], $uid, NULL, '', REQUEST_TIME);
        }
        // if we're copying notification emails, here's where we do one
        if (variable_get('uc_bitpay_copy_notify_emails', FALSE))
        {
            // construct an alert to email
            $params             = array();
            $params['id']       = $notify['id'];
            $params['url']      = $notify['url'];
            $params['order_id'] = $order_id;
            // send the email
            $to      = variable_get('uc_bitpay_alert_email', '');
            $success = drupal_mail('uc_bitpay', 'complete', $to, $language, $params, "BitPay");
        }
        break;

    // EXPIRED: This status reflects that the buyer did not submit
    // full payment within the 15-minute window Bitpay allows, and
    // thus the invoice is no longer to be used. As of 2012-10-31,
    // Bitpay does not actively send a notification when an invoice
    // becomes expired. This code will be left in on the chance that
    // they eventually do.
    case 'expired':
        // do nothing
        break;

    // INVALID: This status occurs when the invoice was considered
    // 'paid', but did not receive a confirmation on the bitcoin
    // blockchain within an hour after receipt of payment. (Note
    // that this is NOT the same as the invoice status becoming
    // 'confirmed'.) This is a very unlikely occurance for any given
    // invoice, and is most likely due to a global slowdown in the
    // bitcoin network's processing. Should this status occur, a
    // message will be sent to the notification email address, in
    // case contact with Bitpay needs to be made. Typically Bitpay
    // simply credits the funds to the merchant account after the
    // blockchain confirmation finally occurs.
    case 'invalid':
        // construct an alert to email
        $params             = array();
        $params['id']       = $notify['id'];
        $params['url']      = $notify['url'];
        $params['order_id'] = $order_id;
        $params['status']   = $notify['status'];
        // send the email
        $to      = variable_get('uc_bitpay_alert_email', '');
        $success = drupal_mail('uc_bitpay', 'invalid', $to, $language, $params, "BitPay");

        uc_order_comment_save($order_id, 0, t("The Bitpay invoice for this order has been marked INVALID. You may neet to contact Bitpay to resolve the issue."), 'admin');
        break;

    // NEW: This should never be sent as a notification; all invoices
    // are created with this status, and invoices do not revert back to
    // it. If this is still the status, there has been no change and no
    // notification should have been sent.
    //
    // OR
    //
    // OTHER: The invoice has been assigned some unknown, either
    // erroneous or newly-implemented	status.
    //
    // Do nothing except alert the owner of the notification email
    // address of this unusual status notification.
    default:
        // construct an alert to email
        $params             = array();
        $params['id']       = $notify['id'];
        $params['url']      = $notify['url'];
        $params['order_id'] = $order_id;
        $params['status']   = $notify['status'];
        // send the email
        $to      = variable_get('uc_bitpay_alert_email', '');
        $success = drupal_mail('uc_bitpay', 'default', $to, $language, $params, "BitPay");
    } // end switch - examining the invoice status
} // end function - uc_bitpay_notification

/**
 * Sends out an email alert from the Bitpay module
 * Implements hook_mail()
 *
 * @param $key
 * @param $message
 * @param $params
 */
function uc_bitpay_mail($key, &$message, $params)
{
    // The key allows for a module to define multiple mails.
    switch($key)
    {
    case 'invalid_api_key':
        $message['subject'] = t("Invalid API Key");
        $message['body'][] = t("Your website has just received a Bitpay invoice status notification with what appears to be an invalid API key. You will want to have your web developer investigate this situation if you are unsure of the reason behind it (such as recently changed API keys.)");
        //$message['body'][] = wordwrap($email_text, 65, "\r\n");
        $message['body'][] = "\r\n";
        break;
    case 'paid':
        $message['subject'] = t("Invoice Paid");
        $message['body'][] = "Bitpay " .
            t("has just updated an invoice status as: 'paid'. This indicates that full payment has been made on the invoice, but the bitcoin transaction has not yet been confirmed.");
        //$message['body'][] = wordwrap($email_text, 65, "\r\n");
        $message['body'][] = "\r\n\r\n" .
            t("Invoice") . " ID: " . $params['id'] . "\r\n" .
            t("Invoice") . " URL: " . $params['url'] . "\r\n" .
            t("Order")   . " ID: " . $params['order_id'] . "\r\n";
        break;
    case 'confirmed':
        $message['subject'] = t("Invoice Confirmed");
        $message['body'][] = "Bitpay " .
            t("has just updated an invoice status as: 'confirmed'. This indicates that the payment has been confirmed according to your Bitpay settings, and fulfillment of the order should occur.");
        //$message['body'][] = wordwrap($email_text, 65, "\r\n");
        $message['body'][] = "\r\n\r\n" .
            t("Invoice") . " ID: " . $params['id'] . "\r\n" .
            t("Invoice") . " URL: " . $params['url'] . "\r\n" .
            t("Order")   . " ID: " . $params['order_id'] . "\r\n";
        break;
    case 'complete':
        $message['subject'] = t("Invoice Complete");
        $message['body'][] = "Bitpay " .
            t("has just updated an invoice status as: 'complete'. This indicates that the sale has been fully confirmed on the bitcoin network and your Bitpay merchant account has been credited.");
        //$message['body'][] = wordwrap($email_text, 65, "\r\n");
        $message['body'][] = "\r\n\r\n" .
            t("Invoice") . " ID: " . $params['id'] . "\r\n" .
            t("Invoice") . " URL: " . $params['url'] . "\r\n" .
            t("Order")   . " ID: " . $params['order_id'] . "\r\n";
        break;
    case 'invalid':
        $message['subject'] = t("Invalid Invoice");
        $message['body'][] = "Bitpay " .
            t("has just updated an invoice status as: '") . $params['status'] .
            t("'. Your payments department should be informed of this status update; Bitpay may need to be contacted to confirm that the merchant account is credited after the payment eventually confirms on the bitcoin network.");
        //$message['body'][] = wordwrap($email_text, 65, "\r\n");
        $message['body'][] = "\r\n\r\n" .
            t("Invoice") . " ID: " . $params['id'] . "\r\n" .
            t("Invoice") . " URL: " . $params['url'] . "\r\n" .
            t("Order")   . " ID: " . $params['order_id'] . "\r\n";
        break;
    case 'default':
        $message['subject'] = t("Unusual Invoice Status");
        $message['body'][] = "Bitpay " .
            t("has just sent an invoice status notification with an unusal status of: '") .
            $params['status'] . t("' to your website. Your website developer should be informed of this status update to ascertain what further action, if any, needs to be taken.");
        //$message['body'][] = wordwrap($email_text, 65, "\r\n");
        $message['body'][] = "\r\n\r\n" .
            t("Invoice") . " ID: " . $notify['id'] . "\r\n" .
            t("Invoice") . " URL: " . $notify['url'] . "\r\n" .
            t("Order")   . " ID: " . $order_id . "\r\n";
        break;
    }
}

/**
 * Returns the string of the 'From' email address to be used when
 * sending out email alerts from the module.
 */
function _get_bp_email_from_address()
{
    return uc_store_email_from();
}

/**
 * Returns the current Bitpay API key; used to authenticate notfications
 */
function _get_bp_key()
{
    return variable_get('uc_bitpay_current_api_key','');
}

/**
 * Returns the previously-used Bitpay API key; used to double-check failed authentications
 */
function _get_prior_bp_key()
{
    return variable_get('uc_bitpay_prior_api_key','');
}
